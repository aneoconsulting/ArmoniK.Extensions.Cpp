// Reproduction test for the WaitResults() download-memory issue: submitting a batch of tasks and
// waiting for all of them at once causes every result that completes within the same polling round
// to be downloaded and dispatched concurrently, each holding its full payload in memory until the
// handler consumes it. Before GrpcClient__DownloadByteBudget existed, nothing bounded how many of
// those payloads could be resident at once beyond thread_pool_'s size (GrpcClient__ThreadPoolSize,
// defaulting to hardware concurrency) -- fine when sizes are uniform, but a poor proxy for the
// resource that actually matters (bytes) once results vary in size.
//
// This test doesn't assert an absolute memory ceiling (baseline RSS and allocator behavior are
// environment-dependent) - it compares the peak RSS delta observed while WaitResults() drains a
// batch under a disabled budget (unbounded, matching pre-existing behavior) against a tight one,
// and asserts the tight budget holds meaningfully less payload data in memory at once.
//
// Requires a deployed ArmoniK cluster with the C++ end2end worker, same as the rest of this test
// binary (see .docs/content/guide/2.tests.md).

#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <functional>
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

// Resident set size of the current process, in KB, or -1 if it could not be determined.
long GetProcessRssKB() {
  std::ifstream status("/proc/self/status");
  std::string line;
  while (std::getline(status, line)) {
    if (line.rfind("VmRSS:", 0) == 0) {
      long kb = 0;
      if (std::sscanf(line.c_str(), "VmRSS: %ld kB", &kb) == 1) {
        return kb;
      }
    }
  }
  return -1;
}

// Runs `work` while sampling RSS on a background thread, and returns the peak RSS observed during
// the call minus the RSS observed just before it started.
long MeasurePeakRssDeltaKB(const std::function<void()> &work) {
  std::atomic<bool> stop{false};
  std::atomic<long> peak{0};
  long baseline = GetProcessRssKB();

  std::thread sampler([&] {
    while (!stop.load(std::memory_order_relaxed)) {
      long delta = GetProcessRssKB() - baseline;
      long prev = peak.load(std::memory_order_relaxed);
      while (delta > prev && !peak.compare_exchange_weak(prev, delta, std::memory_order_relaxed)) {
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  });

  work();

  stop.store(true, std::memory_order_relaxed);
  sampler.join();
  return peak.load();
}

class SizedEchoHandler final : public ArmoniK::Sdk::Client::IServiceInvocationHandler {
public:
  void HandleResponse(const std::string &result_payload, const std::string & /*taskId*/,
                      const std::string & /*result_id*/) override {
    std::lock_guard<std::mutex> _(mutex);
    ++received;
    total_bytes += result_payload.size();
  }
  void HandleError(const std::exception & /*e*/, const std::string & /*taskId*/) override {
    std::lock_guard<std::mutex> _(mutex);
    ++errors;
  }

  std::mutex mutex;
  int received = 0;
  int errors = 0;
  size_t total_bytes = 0;
};

std::tuple<ArmoniK::Sdk::Common::Properties, armonik::api::common::logger::Logger>
init_with_download_byte_budget(std::int64_t download_byte_budget) {
  ArmoniK::Sdk::Common::Configuration config;
  config.add_json_configuration("appsettings.json").add_env_configuration();
  if (config.get("Worker__Type").empty()) {
    config.set("Worker__Type", "End2EndTest");
  }
  config.set("GrpcClient__DownloadByteBudget", std::to_string(download_byte_budget));

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

// Submits kTaskCount same-sized tasks under the given download byte budget, waits for all of them
// at once, and returns the peak RSS delta observed during that wait.
long RunBatchAndMeasurePeak(unsigned int task_count, size_t payload_bytes, std::int64_t download_byte_budget) {
  auto p = init_with_download_byte_budget(download_byte_budget);
  auto &properties = std::get<0>(p);
  auto &logger = std::get<1>(p);

  ArmoniK::Sdk::Client::SessionService service(properties, logger);

  auto handler = std::make_shared<SizedEchoHandler>();
  auto task_ids = service.Submit(generate_sized_payloads(task_count, payload_bytes), handler);
  EXPECT_EQ(task_ids.size(), task_count);

  long peak_kb = MeasurePeakRssDeltaKB([&] { service.WaitResults(); });

  EXPECT_EQ(handler->received, static_cast<int>(task_count));
  EXPECT_EQ(handler->errors, 0);

  service.CloseSession();
  return peak_kb;
}

} // namespace

TEST(DownloadByteBudget, tight_budget_bounds_peak_download_memory) {
  constexpr unsigned int kTaskCount = 64;
  constexpr size_t kPayloadBytes = 1 * 1024 * 1024; // 1 MiB per result

  // Disabled budget: unbounded, matching pre-existing behavior (concurrency limited only by
  // GrpcClient__ThreadPoolSize).
  long unbounded_peak_kb = RunBatchAndMeasurePeak(kTaskCount, kPayloadBytes, 0);

  // Tight budget: room for only a couple of results' worth of payload bytes at once, regardless of
  // how many results are ready in a given polling round or how many threads the pool has.
  long tight_peak_kb = RunBatchAndMeasurePeak(kTaskCount, kPayloadBytes, 2 * static_cast<std::int64_t>(kPayloadBytes));

  std::cout << "Peak RSS delta - unbounded: " << unbounded_peak_kb << " KB, tight budget (2 payloads): "
            << tight_peak_kb << " KB" << std::endl;

  // The tight budget should hold less payload data in memory at once than the unbounded run. Not
  // asserting a specific ratio (e.g. half): peak RSS delta here also includes overhead the budget
  // doesn't bound -- gRPC channel buffers (TLS session state, HTTP/2 flow-control windows,
  // completion queues) for both the list_results status-check wave and the downloads themselves,
  // plus allocator behavior. Against a 1 MiB payload, that overhead can be large enough relative to
  // the budget's ~2 MiB target that a fixed ratio is too strict and environment-dependent; a
  // regression in the gating itself (e.g. the budget becoming a no-op) would instead show up as
  // tight_peak_kb landing at or above unbounded_peak_kb, not merely below some fraction of it.
  EXPECT_LT(tight_peak_kb, unbounded_peak_kb);
}
