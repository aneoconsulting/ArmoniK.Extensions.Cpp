#include <gtest/gtest.h>

#include "InProcessAgent.h"

#include <ServiceManager.h>
#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <armonik/sdk/common/ConventionTask.h>
#include <armonik/sdk/common/TaskOptions.h>
#include <armonik/sdk/worker/ArmoniKSDKInterface.h>
#include <armonik/sdk/worker/ServiceBase.h>
#include <armonik/sdk/worker/TaskContext.h>

#include <cstring>
#include <map>
#include <stdexcept>
#include <string>

using ArmoniK::Sdk::Common::ArmoniKSdkException;
using ArmoniK::Sdk::Common::ConventionTask;
using ArmoniK::Sdk::Common::TaskOptions;
using ArmoniK::Sdk::DynamicWorker::AppId;
using ArmoniK::Sdk::DynamicWorker::ArmoniKFunctionPointers;
using ArmoniK::Sdk::DynamicWorker::ServiceId;
using ArmoniK::Sdk::DynamicWorker::ServiceManager;
using ArmoniK::Sdk::Worker::ServiceBase;
using ArmoniK::Sdk::Worker::TaskContext;
using TestAgent::MakePayload;

namespace {

// The functions of a library, as the DynamicWorker finds them with dlsym. The service is owned by the test.
void *current_service = nullptr;
void *CreateService(const char *, const char *) { return current_service; }
void DestroyService(void *) {}
void *EnterSession(void *, const char *) { return nullptr; }
void LeaveSession(void *, void *) {}

void Report(armonik_callback_t callback, void *context, armonik_status_t status, const std::string &data) {
  callback(context, status, data.data(), data.size());
}

armonik_status_t CallEcho(void *context, void *, void *, const char *, const char *input, size_t size,
                          armonik_callback_t callback) {
  Report(callback, context, ARMONIK_STATUS_OK, "echo:" + std::string(input, size));
  return ARMONIK_STATUS_OK;
}

armonik_status_t CallWithoutResult(void *, void *, void *, const char *, const char *, size_t, armonik_callback_t) {
  return ARMONIK_STATUS_OK;
}

armonik_status_t CallErrorThenOk(void *context, void *, void *, const char *, const char *, size_t,
                                 armonik_callback_t callback) {
  Report(callback, context, ARMONIK_STATUS_ERROR, "invalid input");
  return ARMONIK_STATUS_OK;
}

armonik_status_t CallRetryWithoutMessage(void *, void *, void *, const char *, const char *, size_t,
                                         armonik_callback_t) {
  return ARMONIK_STATUS_RETRY;
}

armonik_status_t ProcessErrorThenOk(void *context, const armonik_task_t *, void *, void *, const char *,
                                    armonik_callback_t callback) {
  Report(callback, context, ARMONIK_STATUS_ERROR, "invalid input");
  return ARMONIK_STATUS_OK;
}

armonik_status_t ProcessRetryWithoutMessage(void *, const armonik_task_t *, void *, void *, const char *,
                                            armonik_callback_t) {
  return ARMONIK_STATUS_RETRY;
}

// A custom armonik_process that writes its output, but leaves a result it created without data or producer
armonik_status_t ProcessWithOrphanResult(void *, const armonik_task_t *task, void *, void *, const char *,
                                         armonik_callback_t) {
  struct Ignore {
    static void Sink(void *, const char *, size_t) {}
  };
  const armonik_string_t name{"orphan", 6};
  task->create_results_metadata(task->agent, &name, 1, nullptr, &Ignore::Sink, &Ignore::Sink);
  const armonik_result_data_t output{task->outputs[0].id, {"1", 1}};
  return task->send_results(task->agent, &output, 1, nullptr, &Ignore::Sink);
}

class EchoService final : public ServiceBase {
public:
  std::string call(void *, const std::string &name, const std::map<std::string, std::string> &inputs) override {
    return name + ":" + inputs.at("x");
  }
};

class TwoOutputsService final : public ServiceBase {
public:
  void process(void *, const std::string &, TaskContext &context) override {
    context.output("q").write(context.input("x").data() + "-q");
    context.output("r").write("r");
  }
};

class DynamicWorkerTest : public TestAgent::InProcessAgentTest {
protected:
  void TearDown() override {
    current_service = nullptr;
    InProcessAgentTest::TearDown();
  }

  // Runs a convention task as the DynamicWorker does once it has loaded the library
  armonik::api::worker::ProcessStatus Run(ServiceBase *service, armonik_call_t call, armonik_process_t process,
                                          const std::map<std::string, std::string> &outputs,
                                          const std::map<std::string, std::string> &dependencies = {{"dep", "7"}}) {
    current_service = service;
    ServiceManager manager(
        ArmoniKFunctionPointers{&CreateService, &DestroyService, &EnterSession, &LeaveSession, call, process},
        ServiceId(AppId{"library", ""}, "namespace", "service"));
    auto &handler = Handler(MakePayload({{"x", "dep"}}, outputs), dependencies);
    const auto task = ConventionTask::Decode(TaskOptions(handler.getTaskOptions()), handler.getPayload(),
                                             handler.getDataDependencies(), handler.getExpectedResults());
    return manager.UseSession("session").Execute(handler, task);
  }
};

} // namespace

// A library built before armonik_process: armonik_call runs single-output tasks
TEST_F(DynamicWorkerTest, FallbackRunsASingleOutputTask) {
  const auto status = Run(nullptr, &CallEcho, nullptr, {{"result", "out"}});
  ASSERT_TRUE(status.ok()) << status.details();
  const auto &result = service.results.at("out");
  EXPECT_EQ(result.rfind("echo:{", 0), 0u) << result;
}

TEST_F(DynamicWorkerTest, FallbackRejectsSeveralOutputs) {
  try {
    Run(nullptr, &CallEcho, nullptr, {{"q", "out-q"}, {"r", "out-r"}});
    FAIL() << "No exception";
  } catch (const ArmoniKSdkException &e) {
    EXPECT_NE(std::string(e.what()).find("armonik_process"), std::string::npos) << e.what();
  }
  EXPECT_TRUE(service.results.empty());
}

TEST_F(DynamicWorkerTest, CallThatGivesNoResultFails) {
  const auto status = Run(nullptr, &CallWithoutResult, nullptr, {{"result", "out"}});
  EXPECT_FALSE(status.ok());
  EXPECT_NE(status.details().find("without giving its result"), std::string::npos) << status.details();
}

// A failure reported through the callback stands, whatever the library returns
TEST_F(DynamicWorkerTest, ReportedErrorStands) {
  auto status = Run(nullptr, &CallErrorThenOk, nullptr, {{"result", "out"}});
  EXPECT_FALSE(status.ok());
  EXPECT_EQ(status.details(), "invalid input");

  EchoService echo;
  status = Run(&echo, &CallEcho, &ProcessErrorThenOk, {{"result", "out"}});
  EXPECT_FALSE(status.ok());
  EXPECT_EQ(status.details(), "invalid input");
}

// A returned retry retries the task, with or without a message
TEST_F(DynamicWorkerTest, ReturnedRetryRetries) {
  EXPECT_THROW(Run(nullptr, &CallRetryWithoutMessage, nullptr, {{"result", "out"}}), std::runtime_error);
  EchoService echo;
  EXPECT_THROW(Run(&echo, &CallEcho, &ProcessRetryWithoutMessage, {{"result", "out"}}), std::runtime_error);
}

TEST_F(DynamicWorkerTest, FailedUploadRetries) {
  service.fail_notify = true;
  EXPECT_THROW(Run(nullptr, &CallEcho, nullptr, {{"result", "out"}}), std::runtime_error);
}

TEST_F(DynamicWorkerTest, ProcessWritesSeveralOutputs) {
  TwoOutputsService two;
  const auto status = Run(&two, &armonik_call, &armonik_process, {{"q", "out-q"}, {"r", "out-r"}});
  ASSERT_TRUE(status.ok()) << status.details();
  EXPECT_EQ(service.results.at("out-q"), "7-q");
  EXPECT_EQ(service.results.at("out-r"), "r");
}

// JSON cannot hold the input: the map overload of call receives it directly
TEST_F(DynamicWorkerTest, BinaryInputReachesTheMapOverload) {
  EchoService echo;
  const std::string binary("\xff\x00\xfe", 3);
  const auto status = Run(&echo, &armonik_call, &armonik_process, {{"result", "out"}}, {{"dep", binary}});
  ASSERT_TRUE(status.ok()) << status.details();
  EXPECT_EQ(service.results.at("out"), "method:" + binary);
}

// The worker checks a custom armonik_process: a result it created has no data and no producer
TEST_F(DynamicWorkerTest, CustomProcessLeavesAResultWithoutProducer) {
  EchoService echo;
  EXPECT_THROW(Run(&echo, &CallEcho, &ProcessWithOrphanResult, {{"result", "out"}}), ArmoniKSdkException);
}
