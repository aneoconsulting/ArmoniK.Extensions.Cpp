// Reproduction tests for the client's two byte-budget backpressure knobs: GrpcClient__DownloadByteBudget
// (WaitResults) and GrpcClient__UploadByteBudget (Submit/SubmitRaw). The download test checks the
// number of result handlers running at once: WaitResults() reserves each result's bytes before its
// handler is spawned and releases them after it returns, so the budget bounds that number exactly.
// Peak RSS would be a noisier proxy, as it depends on how much freed memory the heap already holds.
// The upload test compares elapsed time instead, since Submit() streams from the caller's own
// buffer rather than copying it, so the budget shows up as serialization, not resident memory.
//
// Requires a deployed ArmoniK cluster with the C++ end2end worker, same as the rest of this test
// binary (see .docs/content/guide/2.tests.md).

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <armonik/common/logger/formatter.h>
#include <armonik/common/logger/logger.h>
#include <armonik/common/logger/writer.h>

#include "armonik/sdk/client/IServiceInvocationHandler.h"
#include "armonik/sdk/client/SessionService.h"
#include "armonik/sdk/common/Configuration.h"
#include "armonik/sdk/common/Properties.h"
#include "armonik/sdk/common/TaskOptions.h"
#include <armonik/sdk/common/TaskPayload.h>

namespace {

class SizedEchoHandler final : public ArmoniK::Sdk::Client::IServiceInvocationHandler {
public:
  /**
   * @param hold How long each HandleResponse() call keeps its result, so that concurrent calls overlap
   */
  explicit SizedEchoHandler(std::chrono::milliseconds hold = std::chrono::milliseconds(0)) : hold(hold) {}

  void HandleResponse(const std::string &result_payload, const std::string & /*taskId*/,
                      const std::string & /*result_id*/) override {
    int current = ++in_flight;
    int expected = max_in_flight.load();
    while (current > expected && !max_in_flight.compare_exchange_weak(expected, current)) {
    }
    std::this_thread::sleep_for(hold);
    --in_flight;

    std::lock_guard<std::mutex> _(mutex);
    ++received;
    total_bytes += result_payload.size();
  }
  void HandleError(const std::exception & /*e*/, const std::string & /*taskId*/) override {
    std::lock_guard<std::mutex> _(mutex);
    ++errors;
  }

  std::chrono::milliseconds hold;
  std::atomic<int> in_flight{0};
  std::atomic<int> max_in_flight{0};
  std::mutex mutex;
  int received = 0;
  int errors = 0;
  size_t total_bytes = 0;
};

// Configures a session with both byte budgets pinned explicitly (rather than leaving either to
// whatever add_env_configuration() picks up from the environment), so the download and upload
// tests stay hermetic with respect to each other and to ambient env vars. thread_pool_size > 0 pins
// GrpcClient__ThreadPoolSize too.
std::tuple<ArmoniK::Sdk::Common::Properties, armonik::api::common::logger::Logger>
init_with_byte_budgets(std::int64_t download_byte_budget, std::int64_t upload_byte_budget, int thread_pool_size = 0) {
  ArmoniK::Sdk::Common::Configuration config;
  config.add_json_configuration("appsettings.json").add_env_configuration();
  if (config.get("Worker__Type").empty()) {
    config.set("Worker__Type", "End2EndTest");
  }
  config.set("GrpcClient__DownloadByteBudget", std::to_string(download_byte_budget));
  config.set("GrpcClient__UploadByteBudget", std::to_string(upload_byte_budget));
  if (thread_pool_size > 0) {
    config.set("GrpcClient__ThreadPoolSize", std::to_string(thread_pool_size));
  }

  ArmoniK::Sdk::Common::TaskOptions task_options("libArmoniK.SDK.Worker.Test.so", config.get("WorkerLib__Version"),
                                                 "End2EndTest", "EchoService", config.get("PartitionId"));
  task_options.max_retries = 1;

  return std::make_tuple<ArmoniK::Sdk::Common::Properties, armonik::api::common::logger::Logger>(
      {config, task_options},
      {armonik::api::common::logger::writer_console(), armonik::api::common::logger::formatter_plain(true)});
}

std::vector<ArmoniK::Sdk::Common::TaskPayload> generate_sized_payloads(unsigned int n, size_t payload_bytes) {
  std::vector<ArmoniK::Sdk::Common::TaskPayload> payloads;
  payloads.reserve(n);
  std::string blob(payload_bytes, 'x');
  for (unsigned int i = 0; i < n; ++i) {
    payloads.emplace_back("EchoService", blob);
  }
  return payloads;
}

// Submits task_count same-sized tasks under the given download byte budget (upload budget
// disabled) and thread pool size, waits for all of them at once, and returns the peak number of
// result handlers that ran at once during that wait.
int RunDownloadBatchAndMeasureMaxHandlers(unsigned int task_count, size_t payload_bytes,
                                          std::int64_t download_byte_budget, int thread_pool_size) {
  auto p = init_with_byte_budgets(download_byte_budget, /*upload_byte_budget=*/0, thread_pool_size);
  auto &properties = std::get<0>(p);
  auto &logger = std::get<1>(p);

  ArmoniK::Sdk::Client::SessionService service(properties, logger);

  // Holding each result lets the handlers of one polling round overlap, up to the limit under test
  auto handler = std::make_shared<SizedEchoHandler>(std::chrono::milliseconds(100));
  auto task_ids = service.Submit(generate_sized_payloads(task_count, payload_bytes), handler);
  EXPECT_EQ(task_ids.size(), task_count);

  service.WaitResults();

  EXPECT_EQ(handler->received, static_cast<int>(task_count));
  EXPECT_EQ(handler->errors, 0);

  service.CloseSession();
  return handler->max_in_flight.load();
}

// Submits call_count * tasks_per_call same-sized tasks as call_count concurrent Submit() calls
// (tasks_per_call tasks per call, each call on its own thread), under the given upload byte budget
// (download budget disabled). Unlike downloaded results, a call's payload is never copied by the
// client -- it's streamed from the caller's own buffer in data_chunk_max_size chunks -- so the
// budget mostly bounds concurrency, not resident memory. That shows up as wall-clock time: a tight
// budget forces calls into more, smaller waves. Returns the elapsed time for all calls to complete,
// not including result draining.
long long RunUploadBatchAndMeasureElapsedMs(unsigned int call_count, unsigned int tasks_per_call, size_t payload_bytes,
                                            std::int64_t upload_byte_budget) {
  auto p = init_with_byte_budgets(/*download_byte_budget=*/0, upload_byte_budget);
  auto &properties = std::get<0>(p);
  auto &logger = std::get<1>(p);

  ArmoniK::Sdk::Client::SessionService service(properties, logger);

  auto handler = std::make_shared<SizedEchoHandler>();

  // Pre-generate payloads outside the measured region, so the timing reflects Submit()'s own
  // admission/serialization, not payload construction.
  std::vector<std::vector<ArmoniK::Sdk::Common::TaskPayload>> payloads_per_call;
  payloads_per_call.reserve(call_count);
  for (unsigned int c = 0; c < call_count; ++c) {
    payloads_per_call.push_back(generate_sized_payloads(tasks_per_call, payload_bytes));
  }

  std::vector<std::vector<std::string>> task_ids_per_call(call_count);

  auto start = std::chrono::steady_clock::now();
  {
    std::vector<std::thread> threads;
    threads.reserve(call_count);
    for (unsigned int c = 0; c < call_count; ++c) {
      threads.emplace_back([&, c] { task_ids_per_call[c] = service.Submit(payloads_per_call[c], handler); });
    }
    for (auto &thread : threads) {
      thread.join();
    }
  }
  auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

  std::size_t total_task_ids = 0;
  for (const auto &ids : task_ids_per_call) {
    total_task_ids += ids.size();
  }
  unsigned int task_count = call_count * tasks_per_call;
  EXPECT_EQ(total_task_ids, task_count);

  service.WaitResults();
  EXPECT_EQ(handler->received, static_cast<int>(task_count));
  EXPECT_EQ(handler->errors, 0);

  service.CloseSession();
  return elapsed_ms;
}

} // namespace

TEST(DownloadByteBudget, tight_budget_bounds_peak_download_memory) {
  constexpr unsigned int kTaskCount = 64;
  constexpr size_t kPayloadBytes = 1 * 1024 * 1024; // 1 MiB per result
  constexpr int kThreadPoolSize = 8;
  constexpr int kBudgetPayloads = 2;

  // Disabled budget: handlers limited only by GrpcClient__ThreadPoolSize. Shows that the test can
  // observe more than kBudgetPayloads handlers at once.
  int unbounded_max = RunDownloadBatchAndMeasureMaxHandlers(kTaskCount, kPayloadBytes, 0, kThreadPoolSize);

  // Tight budget: room for only kBudgetPayloads results' worth of payload bytes at once, regardless
  // of how many results are ready in a given polling round or how many threads the pool has.
  int tight_max = RunDownloadBatchAndMeasureMaxHandlers(
      kTaskCount, kPayloadBytes, kBudgetPayloads * static_cast<std::int64_t>(kPayloadBytes), kThreadPoolSize);

  std::cout << "Max result handlers at once - unbounded: " << unbounded_max << ", tight budget (" << kBudgetPayloads
            << " payloads): " << tight_max << std::endl;

  EXPECT_GT(unbounded_max, kBudgetPayloads);
  EXPECT_LE(tight_max, kBudgetPayloads);
}

TEST(UploadByteBudget, tight_budget_serializes_concurrent_uploads) {
  constexpr unsigned int kCallCount = 8;
  constexpr unsigned int kTasksPerCall = 8;         // 64 tasks total, spread across kCallCount concurrent calls
  constexpr size_t kPayloadBytes = 1 * 1024 * 1024; // 1 MiB per task payload
  constexpr std::int64_t kCallBytes =
      static_cast<std::int64_t>(kTasksPerCall) * static_cast<std::int64_t>(kPayloadBytes);

  long long unbounded_ms = RunUploadBatchAndMeasureElapsedMs(kCallCount, kTasksPerCall, kPayloadBytes, 0);

  // Tight budget: room for only 2 calls' worth of bytes in flight at once, which is stricter than
  // submit_admission_'s own per-worker cap, so it forces extra waves on top of that.
  long long tight_ms = RunUploadBatchAndMeasureElapsedMs(kCallCount, kTasksPerCall, kPayloadBytes, 2 * kCallBytes);

  std::cout << "Elapsed - unbounded: " << unbounded_ms << " ms, tight budget (2 calls): " << tight_ms << " ms"
            << std::endl;

  EXPECT_GT(tight_ms, unbounded_ms);
}
