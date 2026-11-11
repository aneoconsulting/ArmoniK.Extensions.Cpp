#pragma once

// Agent of the control plane, in process, to run a worker against a real TaskHandler in the unit tests

#include <gtest/gtest.h>

#include <armonik/sdk/common/TaskOptions.h>
#include <armonik/sdk/common/internal/ConventionPayload.h>
#include <armonik/worker/Worker/TaskHandler.h>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <ftw.h>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace TestAgent {

namespace agent = armonik::api::grpc::v1::agent;

inline std::string ReadFile(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  std::ostringstream content;
  content << file.rdbuf();
  return content.str();
}

inline void WriteFile(const std::string &path, const std::string &content) {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  file << content;
}

// Agent of the control plane, in process: records what the worker sends and replies as configured
class InProcessAgent final : public agent::Agent::Service {
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
    if (fail_notify) {
      return grpc::Status(grpc::StatusCode::UNAVAILABLE, "agent unavailable");
    }
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
  /** NotifyResultData fails, as when the agent is unreachable */
  bool fail_notify = false;
  std::mutex mutex;
  int next_id = 0;
  std::map<std::string, std::string> results;
  std::vector<Task> tasks;
};

inline int RemoveEntry(const char *path, const struct stat *, int, struct FTW *) { return std::remove(path); }

/**
 * @brief Starts an InProcessAgent, and builds the task handlers of convention tasks that talk to it
 */
class InProcessAgentTest : public ::testing::Test {
protected:
  void SetUp() override {
    std::string pattern = ::testing::TempDir() + "in-process-agent-XXXXXX";
    ASSERT_NE(mkdtemp(&pattern[0]), nullptr);
    folder = pattern;
    service.folder = folder;
    grpc::ServerBuilder builder;
    builder.RegisterService(&service);
    server = builder.BuildAndStart();
    stub = agent::Agent::NewStub(server->InProcessChannel(grpc::ChannelArguments()));
  }

  void TearDown() override {
    handlers.clear();
    server->Shutdown();
    nftw(folder.c_str(), RemoveEntry, 16, FTW_DEPTH | FTW_PHYS);
  }

  /**
   * @brief Task handler of a convention task, valid until the end of the test
   * @param payload Payload: its outputs are the expected results of the task
   * @param dependencies Data dependencies (blob id → content)
   */
  armonik::api::worker::TaskHandler &Handler(const ArmoniK::Sdk::Common::ConventionPayload &payload,
                                             const std::map<std::string, std::string> &dependencies = {}) {
    ArmoniK::Sdk::Common::TaskOptions options("app", "1.0", "namespace", "service", "partition");
    options.SetConvention("method");

    requests.emplace_back();
    auto &request = requests.back();
    request.set_communication_token("token");
    request.set_session_id("session");
    request.set_task_id("task");
    *request.mutable_task_options() = static_cast<armonik::api::grpc::v1::TaskOptions>(options);
    request.set_data_folder(folder);
    request.mutable_configuration()->set_data_chunk_max_size(16);
    const auto payload_id = "payload-" + std::to_string(requests.size());
    WriteFile(folder + "/" + payload_id, payload.Serialize());
    request.set_payload_id(payload_id);
    for (const auto &kv : payload.outputs) {
      request.add_expected_output_keys(kv.second);
    }
    for (const auto &kv : dependencies) {
      WriteFile(folder + "/" + kv.first, kv.second);
      request.add_data_dependencies(kv.first);
    }
    handlers.emplace_back(new armonik::api::worker::TaskHandler(*stub, request));
    return *handlers.back();
  }

  InProcessAgent service;
  std::unique_ptr<grpc::Server> server;
  std::unique_ptr<agent::Agent::Stub> stub;
  std::string folder;
  // A task handler refers to its request
  std::list<armonik::api::grpc::v1::worker::ProcessRequest> requests;
  std::list<std::unique_ptr<armonik::api::worker::TaskHandler>> handlers;
};

inline ArmoniK::Sdk::Common::ConventionPayload MakePayload(std::map<std::string, std::string> inputs,
                                                           std::map<std::string, std::string> outputs) {
  ArmoniK::Sdk::Common::ConventionPayload payload;
  payload.inputs = std::move(inputs);
  payload.outputs = std::move(outputs);
  return payload;
}

} // namespace TestAgent
