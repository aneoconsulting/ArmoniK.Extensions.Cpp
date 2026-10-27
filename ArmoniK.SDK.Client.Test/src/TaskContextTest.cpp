#include <gtest/gtest.h>

#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <armonik/sdk/common/BlobDefinition.h>
#include <armonik/sdk/common/DynamicLibrary.h>
#include <armonik/sdk/common/TaskDefinition.h>
#include <armonik/sdk/common/TaskOptions.h>
#include <armonik/sdk/worker/ServiceBase.h>
#include <armonik/sdk/worker/TaskContext.h>
#include <exception>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>

using ArmoniK::Sdk::Common::ArmoniKSdkException;
using ArmoniK::Sdk::Common::BlobDefinition;
using ArmoniK::Sdk::Common::DynamicLibrary;
using ArmoniK::Sdk::Common::TaskDefinition;
using ArmoniK::Sdk::Common::TaskOptions;
using ArmoniK::Sdk::Worker::ServiceBase;
using ArmoniK::Sdk::Worker::TaskAgent;
using ArmoniK::Sdk::Worker::TaskContext;

namespace {

// Records what the context asks of the agent
class FakeAgent final : public TaskAgent {
public:
  FakeAgent() : options("", "", "", "", "partition") {
    options.SetConvention("method");
    options.max_retries = 5;
  }

  const std::string &session_id() const override { return session; }
  const std::string &task_id() const override { return task; }
  const TaskOptions &task_options() const override { return options; }

  std::vector<std::string> create_results_metadata(const std::vector<std::string> &names) override {
    std::vector<std::string> ids;
    for (const auto &name : names) {
      ids.push_back("id-" + std::to_string(next_id++) + "-" + name);
    }
    if (short_reply && !ids.empty()) {
      ids.pop_back();
    }
    created.insert(created.end(), ids.begin(), ids.end());
    return ids;
  }

  void send_results(const std::vector<ResultData> &results) override {
    for (const auto &result : results) {
      sent[result.id] = std::string(result.data, result.size);
    }
  }

  std::vector<std::string> submit_tasks(const TaskOptions &task_options,
                                        const std::vector<TaskRequest> &tasks) override {
    if (submit_error) {
      std::rethrow_exception(submit_error);
    }
    submitted_options.push_back(task_options);
    std::vector<std::string> ids;
    for (const auto &t : tasks) {
      submitted.push_back(t);
      ids.push_back("task-" + std::to_string(submitted.size()));
    }
    return ids;
  }

  std::string session = "session";
  std::string task = "task";
  TaskOptions options;
  int next_id = 0;
  std::vector<std::string> created;
  std::map<std::string, std::string> sent;
  std::vector<TaskOptions> submitted_options;
  std::vector<TaskRequest> submitted;
  std::exception_ptr submit_error;
  bool short_reply = false;
};

class EchoService final : public ServiceBase {
public:
  std::string call(void *, const std::string &name, const std::map<std::string, std::string> &inputs) override {
    return name + ":" + inputs.at("x");
  }
};

// Tells which overload of call the default process used
class BothOverloadsService final : public ServiceBase {
public:
  std::string call(void *, const std::string &, const std::map<std::string, std::string> &) override { return "map"; }
  std::string call(void *, const std::string &, const std::string &) override { return "payload"; }
};

nlohmann::json Payload(const TaskAgent::TaskRequest &request) { return nlohmann::json::parse(request.payload); }

} // namespace

TEST(TaskContext, Inputs) {
  FakeAgent agent;
  TaskContext context(agent, {{"x", "7"}, {"y", "inline"}}, {{"x", "blob-x"}}, {{"result", "out"}});

  EXPECT_EQ(context.input("x").data(), "7");
  EXPECT_EQ(context.input("x").id(), "blob-x");
  EXPECT_TRUE(context.input("y").id().empty());
  EXPECT_FALSE(context.input("x").AsBlobDefinition().IsRawData());
  EXPECT_EQ(context.input("y").AsBlobDefinition().GetData(), "inline");
  EXPECT_THROW(context.input("z"), ArmoniKSdkException);
  EXPECT_THROW(context.output("other"), ArmoniKSdkException);
  EXPECT_EQ(context.task_options().partition_id, "partition");
}

TEST(TaskContext, DefaultProcessWritesTheSingleOutput) {
  FakeAgent agent;
  EchoService service;
  TaskContext context(agent, {{"x", "7"}}, {}, {{"result", "out"}});

  service.process(nullptr, "echo", context);
  context.finish();
  EXPECT_EQ(agent.sent.at("out"), "echo:7");
}

// ConventionExecutor: the map overload, which takes inputs that are not valid UTF-8
TEST(TaskContext, DefaultProcessCallsTheInputsOverload) {
  FakeAgent agent;
  EchoService service;
  TaskContext context(agent, {{"x", std::string("\xff\x00\xfe", 3)}}, {}, {{"result", "out"}},
                      TaskContext::DefaultCall::Inputs);

  service.process(nullptr, "echo", context);
  EXPECT_EQ(agent.sent.at("out"), "echo:" + std::string("\xff\x00\xfe", 3));

  BothOverloadsService both;
  TaskContext inputs(agent, {}, {}, {{"result", "out-i"}}, TaskContext::DefaultCall::Inputs);
  both.process(nullptr, "m", inputs);
  EXPECT_EQ(agent.sent.at("out-i"), "map");
  TaskContext payload(agent, {}, {}, {{"result", "out-p"}}, TaskContext::DefaultCall::Payload);
  both.process(nullptr, "m", payload);
  EXPECT_EQ(agent.sent.at("out-p"), "payload");
}

// DynamicWorker: the payload cannot hold inputs that are not valid UTF-8, which is a failure, not a retry
TEST(TaskContext, DefaultProcessRejectsBinaryInputsInThePayload) {
  FakeAgent agent;
  EchoService service;
  TaskContext context(agent, {{"x", "\xff"}}, {}, {{"result", "out"}}, TaskContext::DefaultCall::Payload);

  EXPECT_THROW(service.process(nullptr, "echo", context), ArmoniKSdkException);
}

TEST(TaskContext, DefaultProcessRejectsSeveralOutputs) {
  FakeAgent agent;
  EchoService service;
  TaskContext context(agent, {{"x", "7"}}, {}, {{"q", "out-q"}, {"r", "out-r"}});

  EXPECT_THROW(service.process(nullptr, "echo", context), ArmoniKSdkException);
}

TEST(TaskContext, OutputWrittenOnce) {
  FakeAgent agent;
  TaskContext context(agent, {}, {}, {{"q", "out-q"}, {"r", "out-r"}});

  context.output("q").write("3");
  EXPECT_THROW(context.output("q").write("4"), ArmoniKSdkException);
  EXPECT_EQ(agent.sent.at("out-q"), "3");
  EXPECT_THROW(context.finish(), ArmoniKSdkException);
  context.output("r").write("2");
  EXPECT_NO_THROW(context.finish());
}

TEST(TaskContext, CreateBlobs) {
  FakeAgent agent;
  TaskContext context(agent, {}, {}, {});

  const auto ids = context.create_blobs({{"a", "data-a"}, {"b", "data-b"}});
  ASSERT_EQ(ids.size(), 2u);
  EXPECT_EQ(agent.sent.at(ids[0]), "data-a");
  EXPECT_EQ(agent.sent.at(ids[1]), "data-b");

  const auto empty = context.create_blobs_metadata({"c"});
  ASSERT_EQ(empty.size(), 1u);
  EXPECT_EQ(agent.sent.count(empty[0]), 0u);
}

TEST(TaskContext, CreateBlobsChecksTheReply) {
  FakeAgent agent;
  agent.short_reply = true;
  TaskContext context(agent, {}, {}, {});

  EXPECT_THROW(context.create_blobs({{"a", "data-a"}, {"b", "data-b"}}), ArmoniKSdkException);
  EXPECT_TRUE(agent.sent.empty());
  EXPECT_THROW(context.create_blobs_metadata({"c"}), ArmoniKSdkException);
}

TEST(TaskContext, SubmitInheritsOptionsAndBuildsPayload) {
  FakeAgent agent;
  agent.options.options[DynamicLibrary::KeyLibraryBlobId] = "library";
  TaskContext context(agent, {{"x", "7"}}, {{"x", "blob-x"}}, {{"result", "out"}});

  const auto ids = context.submit_tasks({TaskDefinition()
                                             .WithInput("raw", BlobDefinition::FromData("data"))
                                             .WithInput("x", context.input("x").AsBlobDefinition())
                                             .WithOutput("q")});
  ASSERT_EQ(ids.size(), 1u);
  ASSERT_EQ(agent.submitted.size(), 1u);
  EXPECT_EQ(agent.submitted_options[0].partition_id, "partition");
  EXPECT_EQ(agent.submitted_options[0].max_retries, 5);
  EXPECT_EQ(agent.submitted_options[0].GetSymbol(), "method");

  const auto &request = agent.submitted[0];
  const auto payload = Payload(request);
  const auto raw_id = payload["inputs"]["raw"].get<std::string>();
  EXPECT_EQ(agent.sent.at(raw_id), "data");
  EXPECT_EQ(payload["inputs"]["x"], "blob-x");
  const auto q_id = payload["outputs"]["q"].get<std::string>();
  EXPECT_EQ(request.expected_outputs, std::vector<std::string>{q_id});
  EXPECT_EQ(request.data_dependencies, (std::vector<std::string>{raw_id, "blob-x", "library"}));
}

TEST(TaskContext, SubmitCreatesDefaultOutput) {
  FakeAgent agent;
  TaskContext context(agent, {}, {}, {});

  context.submit_tasks({TaskDefinition()});
  const auto payload = Payload(agent.submitted[0]);
  ASSERT_EQ(payload["outputs"].size(), 1u);
  EXPECT_TRUE(payload["outputs"].contains(TaskDefinition::DefaultOutputName));
}

TEST(TaskContext, SubtaskOptionsCallAnotherMethod) {
  FakeAgent agent;
  TaskContext context(agent, {}, {}, {});

  const auto options = context.subtask_options("aggregate");
  EXPECT_EQ(options.GetSymbol(), "aggregate");
  EXPECT_EQ(options.partition_id, "partition");

  TaskOptions no_symbol("", "", "", "");
  EXPECT_THROW(context.submit_tasks({TaskDefinition()}, no_symbol), ArmoniKSdkException);

  no_symbol.options[DynamicLibrary::KeySymbol] = "other";
  context.submit_tasks({TaskDefinition()}, no_symbol);
  EXPECT_EQ(agent.submitted_options.back().GetConventionVersion(), DynamicLibrary::ConventionVersion);
}

// HtcMock pattern: the children write new outputs, and the aggregation task takes over the output of the task
TEST(TaskContext, DelegateOutput) {
  FakeAgent agent;
  TaskContext context(agent, {}, {}, {{"result", "out"}});

  const auto children =
      context.submit_tasks({TaskDefinition().WithOutput("result"), TaskDefinition().WithOutput("result")});
  ASSERT_EQ(children.size(), 2u);
  const auto left = Payload(agent.submitted[0])["outputs"]["result"].get<std::string>();
  const auto right = Payload(agent.submitted[1])["outputs"]["result"].get<std::string>();

  context.submit_tasks({TaskDefinition()
                            .WithInput("left", BlobDefinition::FromBlobId(left))
                            .WithInput("right", BlobDefinition::FromBlobId(right))
                            .WithOutput("result", context.output("result").id())},
                       context.subtask_options("aggregate"));
  const auto &aggregate = agent.submitted[2];
  EXPECT_EQ(aggregate.expected_outputs, std::vector<std::string>{"out"});
  EXPECT_EQ(Payload(aggregate)["outputs"]["result"], "out");
  EXPECT_EQ(agent.submitted_options.back().GetSymbol(), "aggregate");

  EXPECT_NO_THROW(context.finish());
  EXPECT_THROW(context.output("result").write("1"), ArmoniKSdkException);
  EXPECT_EQ(agent.sent.count("out"), 0u);
}

TEST(TaskContext, DelegateOutputOnce) {
  FakeAgent agent;
  TaskContext context(agent, {}, {}, {{"result", "out"}, {"written", "out-w"}});

  EXPECT_THROW(context.submit_tasks({TaskDefinition().WithOutput("a", "out"), TaskDefinition().WithOutput("b", "out")}),
               ArmoniKSdkException);
  EXPECT_TRUE(agent.submitted.empty());

  context.output("written").write("1");
  EXPECT_THROW(context.submit_tasks({TaskDefinition().WithOutput("a", "out-w")}), ArmoniKSdkException);

  context.submit_tasks({TaskDefinition().WithOutput("a", "out")});
  EXPECT_THROW(context.submit_tasks({TaskDefinition().WithOutput("a", "out")}), ArmoniKSdkException);
}

TEST(TaskContext, ExistingOutputThatIsNotTheTasks) {
  FakeAgent agent;
  TaskContext context(agent, {}, {}, {{"result", "out"}});

  const auto shared = context.create_blobs_metadata({"shared"});
  context.submit_tasks({TaskDefinition().WithOutput("result", shared[0])});
  EXPECT_EQ(agent.submitted[0].expected_outputs, shared);
  EXPECT_THROW(context.finish(), ArmoniKSdkException);

  EXPECT_THROW(context.submit_tasks({TaskDefinition().WithOutput("result").WithOutput("result", shared[0])}),
               ArmoniKSdkException);

  // A result has a single owner
  const auto other = context.create_blobs_metadata({"other"});
  EXPECT_THROW(
      context.submit_tasks({TaskDefinition().WithOutput("a", other[0]), TaskDefinition().WithOutput("b", other[0])}),
      ArmoniKSdkException);
  EXPECT_THROW(context.submit_tasks({TaskDefinition().WithOutput("a", other[0]).WithOutput("b", other[0])}),
               ArmoniKSdkException);
  EXPECT_EQ(agent.submitted.size(), 1u);
}

// The agent may create some subtasks before it rejects the others: the task must not write a delegated output then
TEST(TaskContext, OutputStaysDelegatedWhenTheSubmissionFails) {
  FakeAgent agent;
  TaskContext context(agent, {}, {}, {{"result", "out"}});

  agent.submit_error = std::make_exception_ptr(ArmoniKSdkException("rejected"));
  EXPECT_THROW(context.submit_tasks({TaskDefinition().WithOutput("result", "out")}), ArmoniKSdkException);
  EXPECT_THROW(context.output("result").write("1"), ArmoniKSdkException);
  EXPECT_EQ(agent.sent.count("out"), 0u);
}

// Nothing but a subtask can write a blob created without data
TEST(TaskContext, BlobWithoutDataMustBeASubtaskOutput) {
  FakeAgent agent;
  TaskContext context(agent, {}, {}, {});

  const auto ids = context.create_blobs_metadata({"shared"});
  context.submit_tasks({TaskDefinition().WithInput("x", BlobDefinition::FromBlobId(ids[0]))});
  EXPECT_THROW(context.finish(), ArmoniKSdkException);

  context.submit_tasks({TaskDefinition().WithOutput("result", ids[0])});
  EXPECT_NO_THROW(context.finish());
  // A result has a single owner
  EXPECT_THROW(context.submit_tasks({TaskDefinition().WithOutput("result", ids[0])}), ArmoniKSdkException);

  // Blobs created with their data need nothing more
  context.create_blobs({{"data", "1"}});
  EXPECT_NO_THROW(context.finish());
}
