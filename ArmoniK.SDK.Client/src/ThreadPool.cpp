#include "ThreadPool.h"

#include <algorithm>
#include <sstream>
#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Client {
namespace Internal {

ThreadPool::Task::Task() = default;

ThreadPool::Task::Task(Function<void()> &&func, ThreadPool::JoinSet *join_set)
    : func_(std::move(func)), join_set_(join_set) {
  if (join_set_) {
    // Increment the task count in the join set
    std::lock_guard<std::mutex> lock(join_set_->mutex_);
    join_set_->task_count_ += 1;
  }
}

ThreadPool::Task::Task(ThreadPool::Task &&other) noexcept : func_(std::move(other.func_)), join_set_(other.join_set_) {
  other.join_set_ = nullptr;
}
ThreadPool::Task &ThreadPool::Task::operator=(ThreadPool::Task &&other) noexcept {
  if (this != &other) {
    func_ = std::move(other.func_);
    join_set_ = other.join_set_;
    other.join_set_ = nullptr;
  }
  return *this;
}

ThreadPool::Task::~Task() {
  if (join_set_) {
    // Decrement the task count in the join set
    std::lock_guard<std::mutex> lock(join_set_->mutex_);
    join_set_->task_count_ -= 1;
    if (join_set_->task_count_ == 0) {
      join_set_->wake_condition_.notify_all();
    }
  }
}

void ThreadPool::Task::Execute(armonik::api::common::logger::ILogger &logger) {

  try {
    func_();
  } catch (const std::exception &e) {
    logger.error("Exception in thread pool task: " + std::string(e.what()));
    RecordError();
  } catch (...) {
    logger.error("Unknown exception in thread pool task");
    RecordError();
  }
}

void ThreadPool::Task::RecordError() {
  if (!join_set_) {
    return;
  }

  std::lock_guard<std::mutex> lock(join_set_->mutex_);

  // Keep only the first exception
  if (!join_set_->exception_) {
    join_set_->exception_ = std::current_exception();
    join_set_->wake_condition_.notify_all();
  }
}

thread_local ThreadPool *ThreadPool::current_pool_ = nullptr;
thread_local ThreadPool::Worker *ThreadPool::current_worker_ = nullptr;

ThreadPool::ThreadPool(int max_threads, armonik::api::common::logger::Logger &logger)
    : max_threads_(max_threads > 0 ? max_threads : std::max(1u, std::thread::hardware_concurrency())), running_(0),
      logger_(logger), stop_(false) {
  Logger().debug("ThreadPool created", {{"max_threads", std::to_string(max_threads_)}});
}

ThreadPool::~ThreadPool() {
  auto logger = Logger();
  logger.verbose("ThreadPool is stopping...");
  { // Notify all idle threads to stop
    std::lock_guard<std::mutex> lock(mutex_);
    stop_ = true;

    logger.verbose("Notifying all threads to stop...");
    for (Worker *worker : ready_) {
      worker->wake.notify_one();
    }
    for (Worker *worker : reserved_) {
      worker->wake.notify_one();
    }
  }

  // Wait for all threads to finish. Threads still draining the queue may create new ones.
  for (std::size_t i = 0;; ++i) {
    Worker *worker;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (i == workers_.size()) {
        break;
      }
      worker = workers_[i].get();
    }
    if (worker->thread.joinable()) {
      worker->thread.join();
    }
  }

  logger.debug("ThreadPool stopped");
}

armonik::api::common::logger::LocalLogger ThreadPool::Logger(armonik::api::common::logger::Context context) {
  context.emplace("thread_pool_id", std::to_string(reinterpret_cast<std::uintptr_t>(this)));
  return logger_.local(std::move(context));
}

void ThreadPool::Run(Worker &worker) {
  current_pool_ = this;
  current_worker_ = &worker;

  std::stringstream ss;
  ss << std::this_thread::get_id();
  armonik::api::common::logger::Context context{{"thread_id", ss.str()}};
  auto logger = Logger(context);

  logger.debug("Thread started");

  std::unique_lock<std::mutex> lock(mutex_);
  while (true) {
    // The thread is Running here

    if (!pending_.empty()) {
      // A Pending thread waits for a slot to resume its task: swap places with it
      Worker *pending = pending_.front();
      pending_.pop_front();
      pending->state = WorkerState::Running;
      pending->wake.notify_one();

      worker.state = WorkerState::Reserved;
      reserved_.push_back(&worker);
      if (!Idle(worker, lock)) {
        break;
      }
      continue;
    }

    if (!pending_tasks_.empty()) {
      {
        Task task = std::move(pending_tasks_.front());
        pending_tasks_.pop();
        lock.unlock();

        auto task_logger = task.join_set_ ? task.join_set_->Logger(context) : Logger(context);
        task_logger.verbose("Got a new task to execute");

        // Execute the task
        task.Execute(task_logger);

        // Task destructor will handle JoinSet bookkeeping, outside of the pool lock
      }

      lock.lock();
      continue;
    }

    // If the stopping of the pool has been requested and there is no more task, exit the thread
    if (stop_) {
      --running_;
      // Idle threads may be waiting for the queue to drain before exiting
      for (Worker *idle : ready_) {
        idle->wake.notify_one();
      }
      for (Worker *idle : reserved_) {
        idle->wake.notify_one();
      }
      break;
    }

    --running_;
    worker.state = WorkerState::Ready;
    ready_.push_back(&worker);
    if (!Idle(worker, lock)) {
      break;
    }
  }
  lock.unlock();

  logger.debug("Thread stopped");
}

bool ThreadPool::Idle(Worker &worker, std::unique_lock<std::mutex> &lock) {
  worker.wake.wait(lock, [&]() { return worker.state == WorkerState::Running || (stop_ && pending_tasks_.empty()); });
  if (worker.state == WorkerState::Running) {
    return true;
  }

  // The pool is stopping: leave the idle list, so that no one hands this thread a task anymore
  auto &idle = worker.state == WorkerState::Ready ? ready_ : reserved_;
  idle.erase(std::find(idle.begin(), idle.end(), &worker));
  return false;
}

void ThreadPool::StartThread() {
  ++running_;
  if (!reserved_.empty()) {
    Worker *worker = reserved_.back();
    reserved_.pop_back();
    worker->state = WorkerState::Running;
    worker->wake.notify_one();
    return;
  }

  workers_.emplace_back(new Worker());
  Worker *worker = workers_.back().get();
  worker->thread = std::thread([this, worker]() { Run(*worker); });
}

void ThreadPool::Block(Worker &worker) {
  std::lock_guard<std::mutex> lock(mutex_);
  --running_;
  worker.state = WorkerState::Blocked;

  // Hand the released slot over, first to a Pending thread, then to a queued task
  if (!pending_.empty()) {
    Worker *pending = pending_.front();
    pending_.pop_front();
    ++running_;
    pending->state = WorkerState::Running;
    pending->wake.notify_one();
  } else if (!pending_tasks_.empty()) {
    StartThread();
  }
}

void ThreadPool::Unblock(Worker &worker) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (running_ < max_threads_) {
    ++running_;
    worker.state = WorkerState::Running;
    if (running_ + ready_.size() > max_threads_) {
      // Keep #Running + #Ready <= max_threads_
      Worker *ready = ready_.back();
      ready_.pop_back();
      ready->state = WorkerState::Reserved;
      reserved_.push_back(ready);
    }
    return;
  }

  // No free slot: wait for a Running thread to hand over its own
  worker.state = WorkerState::Pending;
  pending_.push_back(&worker);
  worker.wake.wait(lock, [&]() { return worker.state == WorkerState::Running; });
}

ThreadPool::BlockingScope::BlockingScope() : pool_(current_pool_), worker_(current_worker_) {
  if (worker_) {
    // Nested scopes are no-ops
    current_worker_ = nullptr;
    pool_->Block(*worker_);
  }
}

ThreadPool::BlockingScope::~BlockingScope() {
  if (worker_) {
    pool_->Unblock(*worker_);
    current_worker_ = worker_;
  }
}

void ThreadPool::Spawn(Task &&task) {
  auto logger = task.join_set_ ? task.join_set_->Logger() : Logger();
  logger.verbose("Spawning new task");

  { // Lock the pool to enqueue a new task
    std::lock_guard<std::mutex> lock(mutex_);
    if (stop_) {
      throw std::runtime_error("Spawn on stopped ThreadPool");
    }

    // Enqueue the task
    pending_tasks_.push(std::move(task));

    if (!ready_.empty()) {
      // A Ready thread already holds a slot
      Worker *worker = ready_.back();
      ready_.pop_back();
      ++running_;
      worker->state = WorkerState::Running;
      worker->wake.notify_one();
    } else if (running_ < max_threads_) {
      StartThread();
    }
    // Otherwise, a Running thread will pick the task once done with its own
  }
}

void ThreadPool::Spawn(Function<void()> &&f) { Spawn(Task(std::move(f))); }

ThreadPool::JoinSet::JoinSet(ThreadPool &thread_pool) : thread_pool_(thread_pool), task_count_(0) {
  Logger().debug("JoinSet created");
}

ThreadPool::JoinSet::~JoinSet() {
  BlockingWait(mutex_, wake_condition_, [this]() { return task_count_ == 0; });

  Logger().debug("JoinSet destroyed");
}

armonik::api::common::logger::LocalLogger ThreadPool::JoinSet::Logger(armonik::api::common::logger::Context context) {
  context.emplace("join_set_id", std::to_string(reinterpret_cast<std::uintptr_t>(this)));
  return thread_pool_.Logger(std::move(context));
}

void ThreadPool::JoinSet::Spawn(Function<void()> &&f) { thread_pool_.Spawn(Task(std::move(f), this)); }

void ThreadPool::JoinSet::Wait() {
  BlockingWait(mutex_, wake_condition_, [this]() { return task_count_ == 0 || exception_; });

  std::lock_guard<std::mutex> lock(mutex_);

  if (exception_) {
    Logger().debug("Rethrow JoinSet error");
    auto e = exception_;
    exception_ = nullptr;
    std::rethrow_exception(e);
  }

  Logger().debug("JoinSet emptied");
}

} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
