#pragma once

#include "ThreadPool.h"
#include <algorithm>
#include <cstddef>
#include <thread>

namespace ArmoniK {
namespace Sdk {
namespace Client {
namespace Internal {

/**
 * @brief Run f(i) for i in [0, count), split across pool when count is large enough to be worth it
 * @param pool The thread pool to run chunks on
 * @param count Number of indices to process
 * @param f Callable invoked with each index; must be safe to call concurrently for distinct indices
 * @param serial_threshold Below this count, run serially on the calling thread instead of using the pool
 */
template <typename F> void ParallelFor(ThreadPool &pool, std::size_t count, F &&f, std::size_t serial_threshold = 64) {
  if (count < serial_threshold) {
    for (std::size_t i = 0; i < count; ++i) {
      f(i);
    }
    return;
  }

  const std::size_t num_chunks =
      std::min<std::size_t>(count, std::max<unsigned int>(1, std::thread::hardware_concurrency()));
  const std::size_t chunk_size = (count + num_chunks - 1) / num_chunks;

  ThreadPool::JoinSet join_set(pool);
  for (std::size_t start = 0; start < count; start += chunk_size) {
    std::size_t end = std::min(start + chunk_size, count);
    join_set.Spawn([&f, start, end]() {
      for (std::size_t i = start; i < end; ++i) {
        f(i);
      }
    });
  }
  join_set.Wait();
}

} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
