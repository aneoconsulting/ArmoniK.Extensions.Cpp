#include <gtest/gtest.h>

#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <armonik/sdk/common/BlobDefinition.h>
#include <armonik/sdk/common/TaskDefinition.h>
#include <armonik/sdk/common/TaskOptions.h>
#include <armonik/sdk/common/internal/ConventionPayload.h>
#include <armonik/sdk/worker/ConventionExecutor.h>
#include <armonik/sdk/worker/ServiceBase.h>
#include <armonik/sdk/worker/TaskContext.h>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <ftw.h>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using ArmoniK::Sdk::Common::ArmoniKSdkException;
using ArmoniK::Sdk::Common::BlobDefinition;
using ArmoniK::Sdk::Common::ConventionPayload;
using ArmoniK::Sdk::Common::TaskDefinition;
using ArmoniK::Sdk::Common::TaskOptions;
using ArmoniK::Sdk::Worker::ConventionExecutor;
using ArmoniK::Sdk::Worker::ServiceBase;
using ArmoniK::Sdk::Worker::TaskContext;
namespace agent = armonik::api::grpc::v1::agent;

namespace {

std::string ReadFile(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  std::ostringstream content;
  content << file.rdbuf();
  return content.str();
}

void WriteFile(const std::string &path, const std::string &content) {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  file << content;
}

// Agent of the control plane, in process: records what the worker sends and replies as configured
class FakeAgent final : public agent::Agent::Service {
public:
  enum class Reply { Created, Rejected, FailedStatus, MissingStatus, Unavailable };

  struct Task {
    std::string payload;
    std::vector<std::string> data_dependencies;
    std::vector<std::string> expected_outputs;
  };

  grpc::Status CreateResultsMetaData(grpc::ServerContext *, const agent::CreateResultsMetaDataRequest *request,
                                     agent::CreateResultsMetaDataResponse *response) override {
    std::lock_guard<std::mutex> lock(mutex);
    for (const auto &result : request->results()) {
      auto *created = response->add_results();
      created->set_session_id(request->session_id());
      created->set_name(result.name());
      created->set_result_id("blob-" + std::to_string(next_id++));
    }
    return grpc::Status::OK;
  }

  grpc::Status NotifyResultData(grpc::ServerContext *, const agent::NotifyResultDataRequest *request,
                                agent::NotifyResultDataResponse *response) override {
    std::lock_guard<std::mutex> lock(mutex);
    for (const auto &id : request->ids()) {
      results[id.result_id()] = ReadFile(folder + "/" + id.result_id());
      response->add_result_ids(id.result_id());
    }
    return grpc::Status::OK;
  }

  grpc::Status CreateTask(grpc::ServerContext *, grpc::ServerReader<agent::CreateTaskRequest> *reader,
                          agent::CreateTaskReply *response) override {
    std::vector<Task> received;
    Task current;
    agent::CreateTaskRequest message;
    while (reader->Read(&message)) {
      if (message.type_case() == agent::CreateTaskRequest::kInitTask && message.init_task().has_header()) {
        const auto &header = message.init_task().header();
        current = Task();
        current.data_dependencies.assign(header.data_dependencies().begin(), header.data_dependencies().end());
        current.expected_outputs.assign(header.expected_output_keys().begin(), header.expected_output_keys().end());
      } else if (message.type_case() == agent::CreateTaskRequest::kTaskPayload) {
        if (message.task_payload().type_case() == armonik::api::grpc::v1::DataChunk::kData) {
          current.payload += message.task_payload().data();
        } else {
          received.push_back(current);
        }
      }
    }
    if (reply == Reply::Unavailable) {
      return grpc::Status(grpc::StatusCode::UNAVAILABLE, "agent unavailable");
    }
    if (reply == Reply::Rejected) {
      response->set_error("rejected by the agent");
      return grpc::Status::OK;
    }
    std::lock_guard<std::mutex> lock(mutex);
    auto *statuses = response->mutable_creation_status_list();
    for (std::size_t i = 0; i < received.size(); ++i) {
      if (reply == Reply::MissingStatus && i + 1 == received.size()) {
        break;
      }
      auto *status = statuses->add_creation_statuses();
      if (reply == Reply::FailedStatus) {
        status->set_error("invalid task");
      } else {
        status->mutable_task_info()->set_task_id("subtask-" + std::to_string(tasks.size()));
        tasks.push_back(received[i]);
      }
    }
    return grpc::Status::OK;
  }

  std::string folder;
  Reply reply = Reply::Created;
  std::mutex mutex;
  int next_id = 0;
  std::map<std::string, std::string> results;
  std::vector<Task> tasks;
};

int RemoveEntry(const char *path, const struct stat *, int, struct FTW *) { return std::remove(path); }

class ConventionExecutorTest : public ::testing::Test {
protected:
  void SetUp() override {
    std::string pattern = ::testing::TempDir() + "convention-executor-XXXXXX";
    ASSERT_NE(mkdtemp(&pattern[0]), nullptr);
    folder = pattern;
    service.folder = folder;
    grpc::ServerBuilder builder;
    builder.RegisterService(&service);
    server = builder.BuildAndStart();
    stub = agent::Agent::NewStub(server->InProcessChannel(grpc::ChannelArguments()));
  }

  void TearDown() override {
    server->Shutdown();
    nftw(folder.c_str(), RemoveEntry, 16, FTW_DEPTH | FTW_PHYS);
  }

  // Runs a convention task with the given payload, expected results and data dependencies (blob id → content)
  armonik::api::worker::ProcessStatus Run(ConventionExecutor &executor, const ConventionPayload &payload,
                                          const std::map<std::string, std::string> &dependencies = {}) {
    TaskOptions options("app", "1.0", "namespace", "service", "partition");
    options.SetConvention("method");

    armonik::api::grpc::v1::worker::ProcessRequest request;
    request.set_communication_token("token");
    request.set_session_id("session");
    request.set_task_id("task");
    *request.mutable_task_options() = static_cast<armonik::api::grpc::v1::TaskOptions>(options);
    request.set_data_folder(folder);
    request.mutable_configuration()->set_data_chunk_max_size(16);
    WriteFile(folder + "/payload", payload.Serialize());
    request.set_payload_id("payload");
    for (const auto &kv : payload.outputs) {
      request.add_expected_output_keys(kv.second);
    }
    for (const auto &kv : dependencies) {
      WriteFile(folder + "/" + kv.first, kv.second);
      request.add_data_dependencies(kv.first);
    }

    armonik::api::worker::TaskHandler handler(*stub, request);
    return executor.Execute(handler);
  }

  FakeAgent service;
  std::unique_ptr<grpc::Server> server;
  std::unique_ptr<agent::Agent::Stub> stub;
  std::string folder;
};

ConventionPayload MakePayload(std::map<std::string, std::string> inputs, std::map<std::string, std::string> outputs) {
  ConventionPayload payload;
  payload.inputs = std::move(inputs);
  payload.outputs = std::move(outputs);
  return payload;
}

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

  service.reply = FakeAgent::Reply::Rejected;
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

  service.reply = FakeAgent::Reply::FailedStatus;
  auto status = Run(executor, payload, {{"dep", "7"}});
  EXPECT_FALSE(status.ok());
  EXPECT_NE(status.details().find("invalid task"), std::string::npos) << status.details();

  service.reply = FakeAgent::Reply::MissingStatus;
  status = Run(executor, payload, {{"dep", "7"}});
  EXPECT_FALSE(status.ok());
  EXPECT_NE(status.details().find("instead of 1"), std::string::npos) << status.details();
}

// A transport failure propagates, so that ArmoniK retries the task, even if the service catches it
TEST_F(ConventionExecutorTest, TransportFailurePropagates) {
  DelegatingService delegating;
  delegating.catch_failure = true;
  ConventionExecutor executor(delegating);

  service.reply = FakeAgent::Reply::Unavailable;
  EXPECT_THROW(Run(executor, MakePayload({{"x", "dep"}}, {{"result", "out"}}), {{"dep", "7"}}), std::runtime_error);
}
