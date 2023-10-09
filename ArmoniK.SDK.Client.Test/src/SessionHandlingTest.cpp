#include <grpcpp/create_channel.h>
#include <gtest/gtest.h>
#include <iostream>

#include <armonik/common/logger/formatter.h>
#include <armonik/common/logger/logger.h>
#include <armonik/common/logger/writer.h>
#include <armonik/common/options/ControlPlane.h>

#include "armonik/sdk/client/SessionService.h"
#include "armonik/sdk/common/Configuration.h"
#include "armonik/sdk/common/Properties.h"
#include "armonik/sdk/common/TaskOptions.h"
#include "armonik/sdk/common/TaskPayload.h"

#include <armonik/client/results_service.grpc.pb.h>
#include <armonik/client/sessions_service.grpc.pb.h>
#include <armonik/client/tasks_service.grpc.pb.h>
#include <armonik/common/tasks_filters.pb.h>

#include "ChannelPool.h"
#include "End2EndHandlers.h"

armonik::api::grpc::v1::tasks::ListTasksRequest::Sort get_default_task_sort() {
  armonik::api::grpc::v1::tasks::ListTasksRequest_Sort sort;
  sort.set_direction(armonik::api::grpc::v1::sort_direction::SORT_DIRECTION_ASC);
  sort.mutable_field()->mutable_task_summary_field()->set_field(
      armonik::api::grpc::v1::tasks::TASK_SUMMARY_ENUM_FIELD_CREATED_AT);
  return sort;
}

armonik::api::grpc::v1::tasks::Filters get_filter_for_session_id(const std::string &session_id) {
  armonik::api::grpc::v1::tasks::Filters filters;
  armonik::api::grpc::v1::tasks::FilterField filterField;
  filterField.mutable_field()->mutable_task_summary_field()->set_field(
      armonik::api::grpc::v1::tasks::TASK_SUMMARY_ENUM_FIELD_SESSION_ID);
  filterField.mutable_filter_string()->set_operator_(armonik::api::grpc::v1::FILTER_STRING_OPERATOR_EQUAL);
  filterField.mutable_filter_string()->set_value(session_id);
  *filters.mutable_or_()->Add()->mutable_and_()->Add() = filterField;
  return filters;
}

std::vector<ArmoniK::Sdk::Common::TaskPayload> generate_payloads(unsigned int n) {
  std::vector<ArmoniK::Sdk::Common::TaskPayload> payloads;
  payloads.reserve(n);
  for (unsigned int i = 0; i < n; ++i) {
    payloads.emplace_back("EchoService", "Test");
  }
  return payloads;
}

std::tuple<ArmoniK::Sdk::Common::Properties, armonik::api::common::logger::Logger> init() {
  ArmoniK::Sdk::Common::Configuration config;
  config.add_json_configuration("appsettings.json").add_env_configuration();

  std::cout << "Endpoint : " << config.get("Grpc__EndPoint") << std::endl;
  if (config.get("Worker__Type").empty()) {
    config.set("Worker__Type", "End2EndTest");
  }

  // Create the task options
  ArmoniK::Sdk::Common::TaskOptions session_task_options(
      "libArmoniK.SDK.Worker.Test.so", config.get("WorkerLib__Version"), "End2EndTest", "EchoService");
  session_task_options.max_retries = 1;

  return std::make_tuple<ArmoniK::Sdk::Common::Properties, armonik::api::common::logger::Logger>(
      {config, session_task_options},
      {armonik::api::common::logger::writer_console(), armonik::api::common::logger::formatter_plain(true)});
}

std::shared_ptr<::grpc::ClientContext> get_context() { return std::make_shared<::grpc::ClientContext>(); }

TEST(SessionService, reopen_test) {
  auto p = init();
  auto properties = std::move(std::get<0>(p));
  auto logger = std::move(std::get<1>(p));
  ArmoniK::Sdk::Client::Internal::ChannelPool pool(properties, logger);
  auto channel_guard = pool.GetChannel();

  // Create service #1
  ArmoniK::Sdk::Client::SessionService service(properties, logger);
  const auto &current_session = service.getSession();
  ASSERT_FALSE(current_session.empty());

  // Verify it has no tasks
  armonik::api::grpc::v1::tasks::ListTasksRequest request;
  armonik::api::grpc::v1::tasks::ListTasksResponse response;
  auto stub = armonik::api::grpc::v1::tasks::Tasks::NewStub(channel_guard.channel);
  *request.mutable_filters() = get_filter_for_session_id(current_session);
  *request.mutable_sort() = get_default_task_sort();
  request.set_page(0);
  request.set_page_size(10);
  request.set_with_errors(true);
  auto status = stub->ListTasks(get_context().get(), request, &response);

  ASSERT_TRUE(status.ok());
  ASSERT_EQ(response.total(), 0);
  response.clear_total();
  response.clear_tasks();

  // Send 1 task
  auto handler = std::make_shared<EchoServiceHandler>(logger);
  auto task_ids = service.Submit(generate_payloads(1), handler);
  ASSERT_EQ(task_ids.size(), 1);

  // Verify the session has 1 task
  status = stub->ListTasks(get_context().get(), request, &response);

  ASSERT_TRUE(status.ok());
  ASSERT_EQ(response.total(), 1);
  response.clear_total();
  response.clear_tasks();

  // Create session #2
  ArmoniK::Sdk::Client::SessionService service_new(properties, logger);
  const auto &new_session = service_new.getSession();
  ASSERT_FALSE(new_session.empty());
  ASSERT_NE(current_session, new_session);

  // Verify the session has 0 task
  *request.mutable_filters() = get_filter_for_session_id(new_session);
  status = stub->ListTasks(get_context().get(), request, &response);

  ASSERT_TRUE(status.ok());
  ASSERT_EQ(response.total(), 0);
  response.clear_total();
  response.clear_tasks();

  // Reopen old session and submit another task
  ArmoniK::Sdk::Client::SessionService service_old(properties, logger, current_session);
  ASSERT_EQ(service_old.getSession(), current_session);
  task_ids = service.Submit(generate_payloads(1), handler);
  ASSERT_EQ(task_ids.size(), 1);

  // Verify the session has 2 tasks
  *request.mutable_filters() = get_filter_for_session_id(current_session);
  status = stub->ListTasks(get_context().get(), request, &response);
  std::cout << status.error_message() << std::endl;
  ASSERT_TRUE(status.ok());
  ASSERT_EQ(response.total(), 2);
}

TEST(SessionService, drop_after_done_test) {
  auto p = init();
  auto properties = std::move(std::get<0>(p));
  auto logger = std::move(std::get<1>(p));
  ArmoniK::Sdk::Client::Internal::ChannelPool pool(properties, logger);
  auto channel_guard = pool.GetChannel();

  ArmoniK::Sdk::Client::SessionService service(properties, logger);
  const auto &session = service.getSession();
  ASSERT_FALSE(session.empty());

  auto handler = std::make_shared<EchoServiceHandler>(logger);
  auto task_ids = service.Submit(generate_payloads(1), handler);
  ASSERT_EQ(task_ids.size(), 1);

  service.WaitResults();
  ASSERT_TRUE(handler->received);
  ASSERT_FALSE(handler->is_error);
  handler->received = false;
  handler->is_error = false;
  service.DropSession();

  auto session_service = armonik::api::grpc::v1::sessions::Sessions::NewStub(channel_guard.channel);

  armonik::api::grpc::v1::sessions::GetSessionRequest request;
  request.set_session_id(session);
  armonik::api::grpc::v1::sessions::GetSessionResponse response;
  auto status = session_service->GetSession(get_context().get(), request, &response);

  ASSERT_TRUE(status.ok());
  ASSERT_EQ(response.session().status(), armonik::api::grpc::v1::session_status::SESSION_STATUS_CANCELLED);

  // Either throws
  try {
    service.Submit(generate_payloads(1), handler);
  } catch (const std::exception &e) {
    return;
  }
  // Or received tasks are cancelled
  service.WaitResults();
  ASSERT_TRUE(handler->received);
  ASSERT_TRUE(handler->is_error);
}

TEST(SessionService, drop_before_done_test) {
  auto p = init();
  auto properties = std::move(std::get<0>(p));
  auto logger = std::move(std::get<1>(p));
  ArmoniK::Sdk::Client::Internal::ChannelPool pool(properties, logger);
  auto channel_guard = pool.GetChannel();

  // Create service
  ArmoniK::Sdk::Client::SessionService service(properties, logger);
  const auto &session = service.getSession();
  ASSERT_FALSE(session.empty());

  // Submit 100 tasks
  auto handler = std::make_shared<EchoServiceHandler>(logger);
  auto task_ids = service.Submit(generate_payloads(100), handler);
  ASSERT_EQ(task_ids.size(), 100);

  // Drop Session before finished
  service.DropSession();
  handler->received = false;
  handler->is_error = false;

  // Shouldn't wait for anything
  service.WaitResults();
  ASSERT_FALSE(handler->received);
  ASSERT_FALSE(handler->is_error);
}

TEST(SessionService, cleanup_tasks) {
  auto p = init();
  auto properties = std::move(std::get<0>(p));
  auto logger = std::move(std::get<1>(p));
  ArmoniK::Sdk::Client::Internal::ChannelPool pool(properties, logger);
  auto channel_guard = pool.GetChannel();

  // Create service
  ArmoniK::Sdk::Client::SessionService service(properties, logger);
  const auto &session = service.getSession();
  ASSERT_FALSE(session.empty());

  // Submit 2 tasks
  auto handler = std::make_shared<EchoServiceHandler>(logger);
  auto task_ids = service.Submit(generate_payloads(2), handler);
  ASSERT_EQ(task_ids.size(), 2);

  // Wait for them to finish
  service.WaitResults();
  ASSERT_TRUE(handler->received);
  ASSERT_FALSE(handler->is_error);

  // Cleanup only the first one
  service.CleanupTasks({task_ids[0]});
  auto stub = armonik::api::grpc::v1::tasks::Tasks::NewStub(channel_guard.channel);
  armonik::api::grpc::v1::tasks::GetResultIdsRequest request;
  armonik::api::grpc::v1::tasks::GetResultIdsResponse response;
  request.mutable_task_id()->Add(task_ids.begin(), task_ids.end());

  // Get the result ids
  auto status = stub->GetResultIds(get_context().get(), request, &response);
  ASSERT_TRUE(status.ok());

  auto result_stub = armonik::api::grpc::v1::results::Results::NewStub(channel_guard.channel);
  armonik::api::grpc::v1::results::DownloadResultDataRequest download_request;
  armonik::api::grpc::v1::results::DownloadResultDataResponse download_response;
  *download_request.mutable_session_id() = session;

  // try to download the result data
  for (auto &&mr : response.task_results()) {
    auto client_context = get_context();
    *download_request.mutable_result_id() = mr.result_ids()[0];
    auto streaming_call = result_stub->DownloadResultData(client_context.get(), download_request);
    while (streaming_call->Read(&download_response)) {
    }
    if (mr.task_id() == task_ids[0]) {
      // Should be empty for the cleaned up one
      ASSERT_TRUE(download_response.data_chunk().empty());
    } else {
      // Shouldn't be empty for the non-cleaned up one
      ASSERT_FALSE(download_response.data_chunk().empty());
    }
    download_response.clear_data_chunk();
  }
}

TEST(WaitOption, timeout_test) {
  auto p = init();
  auto properties = std::move(std::get<0>(p));
  auto logger = std::move(std::get<1>(p));
  ArmoniK::Sdk::Client::Internal::ChannelPool pool(properties, logger);
  auto channel_guard = pool.GetChannel();

  // Create service
  ArmoniK::Sdk::Client::SessionService service(properties, logger);
  const auto &session = service.getSession();
  ASSERT_FALSE(session.empty());

  // Submit 500 tasks
  auto handler = std::make_shared<EchoServiceHandler>(logger);
  auto task_ids = service.Submit(generate_payloads(500), handler);
  ASSERT_EQ(task_ids.size(), 500);

  // Wait with a very short delay
  ArmoniK::Sdk::Client::WaitOptions waitOptions;
  waitOptions.timeout = 100;
  service.WaitResults({}, ArmoniK::Sdk::Client::All, waitOptions);
  handler->received = false;

  // Wait for the rest, should still have tasks to retrieve
  service.WaitResults({}, ArmoniK::Sdk::Client::All);
  ASSERT_TRUE(handler->received);
}
