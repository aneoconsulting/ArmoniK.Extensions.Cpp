#include <gtest/gtest.h>

#include "InProcessAgent.h"

#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <armonik/sdk/common/BlobDefinition.h>
#include <armonik/sdk/common/TaskDefinition.h>
#include <armonik/sdk/common/internal/ConventionPayload.h>
#include <armonik/sdk/worker/ConventionExecutor.h>
#include <armonik/sdk/worker/ServiceBase.h>
#include <armonik/sdk/worker/TaskContext.h>

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using ArmoniK::Sdk::Common::ArmoniKSdkException;
using ArmoniK::Sdk::Common::BlobDefinition;
using ArmoniK::Sdk::Common::ConventionPayload;
using ArmoniK::Sdk::Common::TaskDefinition;
using ArmoniK::Sdk::Worker::ConventionExecutor;
using ArmoniK::Sdk::Worker::ServiceBase;
using ArmoniK::Sdk::Worker::TaskContext;
using TestAgent::InProcessAgent;
using TestAgent::MakePayload;

namespace {

class ConventionExecutorTest : public TestAgent::InProcessAgentTest {
protected:
  // Runs a convention task with the given payload and data dependencies (blob id → content)
  armonik::api::worker::ProcessStatus Run(ConventionExecutor &executor, const ConventionPayload &payload,
                                          const std::map<std::string, std::string> &dependencies = {}) {
    return executor.Execute(Handler(payload, dependencies));
  }
};

class EchoService final : public ServiceBase {
public:
  std::string call(void *, const std::string &name, const std::map<std::string, std::string> &inputs) override {
    return name + ":" + inputs.at("x");
  }
};

// Writes q, and r unless told not to
class TwoOutputsService final : public ServiceBase {
public:
  std::string call(void *, const std::string &, const std::map<std::string, std::string> &) override { return ""; }
  void process(void *, const std::string &, TaskContext &context) override {
    context.output("q").write(context.input("x").data() + "-q");
    if (write_r) {
      context.output("r").write("r");
    }
  }
  bool write_r = true;
};

// Delegates its output to a subtask that reads its input, and catches the failure of the submission if told to
class DelegatingService final : public ServiceBase {
public:
  std::string call(void *, const std::string &, const std::map<std::string, std::string> &) override { return ""; }
  void process(void *, const std::string &, TaskContext &context) override {
    const std::vector<TaskDefinition> subtasks{TaskDefinition()
                                                   .WithInput("x", context.input("x").AsBlobDefinition())
                                                   .WithInput("y", BlobDefinition::FromData(std::string(100, 'y')))
                                                   .WithOutput("result", context.output("result").id())};
    if (!catch_failure) {
      context.submit_tasks(subtasks);
      return;
    }
    try {
      context.submit_tasks(subtasks);
    } catch (const std::exception &) {
      // The service goes on as if nothing happened: the executor must still fail the task
    }
  }
  bool catch_failure = false;
};

} // namespace

TEST_F(ConventionExecutorTest, DefaultProcessWritesTheSingleOutput) {
  EchoService echo;
  ConventionExecutor executor(echo);

  const auto status = Run(executor, MakePayload({{"x", "dep"}}, {{"result", "out"}}), {{"dep", "7"}});
  ASSERT_TRUE(status.ok()) << status.details();
  EXPECT_EQ(service.results.at("out"), "method:7");
}

TEST_F(ConventionExecutorTest, WritesSeveralOutputs) {
  TwoOutputsService two;
  ConventionExecutor executor(two);

  const auto status = Run(executor, MakePayload({{"x", "7"}}, {{"q", "out-q"}, {"r", "out-r"}}));
  ASSERT_TRUE(status.ok()) << status.details();
  EXPECT_EQ(service.results.at("out-q"), "7-q");
  EXPECT_EQ(service.results.at("out-r"), "r");
}

TEST_F(ConventionExecutorTest, FailsWhenAnOutputIsMissing) {
  TwoOutputsService two;
  two.write_r = false;
  ConventionExecutor executor(two);

  const auto status = Run(executor, MakePayload({{"x", "7"}}, {{"q", "out-q"}, {"r", "out-r"}}));
  EXPECT_FALSE(status.ok());
  EXPECT_NE(status.details().find("'r'"), std::string::npos) << status.details();
}

TEST_F(ConventionExecutorTest, DefaultProcessRejectsSeveralOutputs) {
  EchoService echo;
  ConventionExecutor executor(echo);

  const auto status = Run(executor, MakePayload({{"x", "7"}}, {{"q", "out-q"}, {"r", "out-r"}}));
  EXPECT_FALSE(status.ok());
  EXPECT_TRUE(service.results.empty());
}

TEST_F(ConventionExecutorTest, DelegatesAnOutputToASubtask) {
  DelegatingService delegating;
  ConventionExecutor executor(delegating);

  const auto status = Run(executor, MakePayload({{"x", "dep"}}, {{"result", "out"}}), {{"dep", "7"}});
  ASSERT_TRUE(status.ok()) << status.details();
  ASSERT_EQ(service.tasks.size(), 1u);
  const auto &subtask = service.tasks[0];
  EXPECT_EQ(subtask.expected_outputs, std::vector<std::string>{"out"});
  // The payload is longer than a chunk: it arrives whole
  const auto payload = ConventionPayload::Deserialize(subtask.payload);
  EXPECT_EQ(payload.outputs, (std::map<std::string, std::string>{{"result", "out"}}));
  EXPECT_EQ(payload.inputs.at("x"), "dep");
  const auto y = payload.inputs.at("y");
  EXPECT_EQ(service.results.at(y), std::string(100, 'y'));
  EXPECT_EQ(subtask.data_dependencies, (std::vector<std::string>{y, "dep"}));
  EXPECT_EQ(service.results.count("out"), 0u);
}

// A rejection fails the task, without retry, even if the service catches the exception
TEST_F(ConventionExecutorTest, FailsWhenTheAgentRejectsTheSubtasks) {
  DelegatingService delegating;
  ConventionExecutor executor(delegating);
  const auto payload = MakePayload({{"x", "dep"}}, {{"result", "out"}});

  service.reply = InProcessAgent::Reply::Rejected;
  auto status = Run(executor, payload, {{"dep", "7"}});
  EXPECT_FALSE(status.ok());
  EXPECT_NE(status.details().find("rejected by the agent"), std::string::npos) << status.details();

  delegating.catch_failure = true;
  status = Run(executor, payload, {{"dep", "7"}});
  EXPECT_FALSE(status.ok());
  EXPECT_NE(status.details().find("rejected by the agent"), std::string::npos) << status.details();
}

TEST_F(ConventionExecutorTest, FailsWhenASubtaskIsNotCreated) {
  DelegatingService delegating;
  delegating.catch_failure = true;
  ConventionExecutor executor(delegating);
  const auto payload = MakePayload({{"x", "dep"}}, {{"result", "out"}});

  service.reply = InProcessAgent::Reply::FailedStatus;
  auto status = Run(executor, payload, {{"dep", "7"}});
  EXPECT_FALSE(status.ok());
  EXPECT_NE(status.details().find("invalid task"), std::string::npos) << status.details();

  service.reply = InProcessAgent::Reply::MissingStatus;
  status = Run(executor, payload, {{"dep", "7"}});
  EXPECT_FALSE(status.ok());
  EXPECT_NE(status.details().find("instead of 1"), std::string::npos) << status.details();
}

// A transport failure propagates, so that ArmoniK retries the task, even if the service catches it
TEST_F(ConventionExecutorTest, TransportFailurePropagates) {
  DelegatingService delegating;
  delegating.catch_failure = true;
  ConventionExecutor executor(delegating);

  service.reply = InProcessAgent::Reply::Unavailable;
  EXPECT_THROW(Run(executor, MakePayload({{"x", "dep"}}, {{"result", "out"}}), {{"dep", "7"}}), std::runtime_error);
}
