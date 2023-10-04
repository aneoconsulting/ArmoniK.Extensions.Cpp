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

#include <armonik/client/sessions_service.grpc.pb.h>
#include <armonik/client/tasks_service.grpc.pb.h>
#include <armonik/common/session_status.pb.h>
#include <armonik/common/sort_direction.pb.h>
#include <armonik/common/tasks_filters.pb.h>

#include "End2EndHandlers.h"

std::unique_ptr<armonik::api::grpc::v1::sessions::Sessions::Stub>
get_session_stub(const ArmoniK::Sdk::Common::Properties &properties) {
  return armonik::api::grpc::v1::sessions::Sessions::NewStub(::grpc::CreateChannel(
      std::string(properties.configuration.get_control_plane().getEndpoint()), ::grpc::InsecureChannelCredentials()));
}

std::unique_ptr<armonik::api::grpc::v1::tasks::Tasks::Stub>
get_task_stub(const ArmoniK::Sdk::Common::Properties &properties) {
  return armonik::api::grpc::v1::tasks::Tasks::NewStub(::grpc::CreateChannel(
      std::string(properties.configuration.get_control_plane().getEndpoint()), ::grpc::InsecureChannelCredentials()));
}

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

  // Create service #1
  ArmoniK::Sdk::Client::SessionService service(properties, logger);
  const auto &current_session = service.getSession();
  ASSERT_FALSE(current_session.empty());

  // Verify it has no tasks
  armonik::api::grpc::v1::tasks::ListTasksRequest request;
  armonik::api::grpc::v1::tasks::ListTasksResponse response;
  auto stub = get_task_stub(properties);
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
  auto handler = std::make_shared<EchoServiceHandler>();
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

  ASSERT_TRUE(status.ok());
  ASSERT_EQ(response.total(), 2);
}

TEST(SessionService, drop_after_done_test) {
  auto p = init();
  auto properties = std::move(std::get<0>(p));
  auto logger = std::move(std::get<1>(p));

  ArmoniK::Sdk::Client::SessionService service(properties, logger);
  const auto &session = service.getSession();
  ASSERT_FALSE(session.empty());

  auto handler = std::make_shared<EchoServiceHandler>();
  auto task_ids = service.Submit(generate_payloads(1), handler);
  ASSERT_EQ(task_ids.size(), 1);

  service.WaitResults();
  ASSERT_TRUE(handler->received);
  ASSERT_FALSE(handler->is_error);
  handler->received = false;
  handler->is_error = false;
  service.DropSession();

  auto session_service = get_session_stub(properties);

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

  ArmoniK::Sdk::Client::SessionService service(properties, logger);
  const auto &session = service.getSession();
  ASSERT_FALSE(session.empty());

  auto handler = std::make_shared<EchoServiceHandler>();
  auto task_ids = service.Submit(generate_payloads(100), handler);
  ASSERT_EQ(task_ids.size(), 100);

  service.DropSession();
  handler->received = false;
  handler->is_error = false;
  service.WaitResults();
  ASSERT_FALSE(handler->received);
  ASSERT_FALSE(handler->is_error);
}
