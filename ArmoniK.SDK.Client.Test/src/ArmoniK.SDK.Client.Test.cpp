#include <cstring>
#include <gtest/gtest.h>
#include <iostream>

#include <algorithm>
#include <armonik/common/logger/formatter.h>
#include <armonik/common/logger/logger.h>
#include <armonik/common/logger/writer.h>
#include <armonik/sdk/client/IServiceInvocationHandler.h>
#include <armonik/sdk/client/SessionService.h>
#include <armonik/sdk/common/Configuration.h>
#include <armonik/sdk/common/Properties.h>
#include <armonik/sdk/common/TaskPayload.h>
#include <chrono>
#include <cmath>
#include <numeric>

#include "End2EndHandlers.h"

template <typename T> std::string StrSerialize(T a, T b) {
  char payload_val[sizeof(T) * 2];

  std::memcpy(payload_val, &a, sizeof(T));
  std::memcpy(payload_val + sizeof(T), &b, sizeof(T));

  return std::string(payload_val, sizeof(T) * 2);
}

std::vector<double> compute_workload(const std::vector<double> &input, const std::size_t nbOutputBytes,
                                     const std::uint32_t workLoadTimeInMs) {
  using clock = std::chrono::high_resolution_clock;
  using milliseconds = std::chrono::milliseconds;

  if (input.empty() || nbOutputBytes <= 0) {
    return std::vector<double>{};
  }

  double result = 0.;
  for (auto x : input) {
    result += x * x * x;
  }
  std::vector<double> output(nbOutputBytes / 8, 0);
  const std::size_t output_size = output.size();

  const auto end = clock::now() + milliseconds(workLoadTimeInMs);
  while (clock::now() <= end) {
    for (std::size_t i = 0; i < output_size; ++i) {
      output[i] = result / static_cast<double>(output_size);
    }
  }
  return output;
}

template <typename T> void serialize_item(char *&outputVec, const T *item, const std::size_t nbItem) {
  std::memcpy(outputVec, item, sizeof(T) * nbItem);
  outputVec += sizeof(T) * nbItem;
}
template <typename T> void serialize_item(char *&outputVec, const T &item) { serialize_item(outputVec, &item, 1); }

TEST(testSDK, testEcho) {
  std::cout << "Hello, World!" << std::endl;
  // Load configuration from file and environment
  ArmoniK::Sdk::Common::Configuration config;
  config.add_json_configuration("appsettings.json").add_env_configuration();

  std::cout << "Endpoint : " << config.get("GrpcClient__Endpoint") << std::endl;
  if (config.get("Worker__Type").empty()) {
    config.set("Worker__Type", "End2EndTest");
  }

  // Create the task options
  ArmoniK::Sdk::Common::TaskOptions session_task_options(
      "libArmoniK.SDK.Worker.Test.so", config.get("WorkerLib__Version"), "End2EndTest", "EchoService");
  session_task_options.max_retries = 1;

  // Create the properties
  ArmoniK::Sdk::Common::Properties properties{config, session_task_options};

  // Create the logger
  armonik::api::common::logger::Logger logger{armonik::api::common::logger::writer_console(),
                                              armonik::api::common::logger::formatter_plain(true)};

  // Create the session service
  ArmoniK::Sdk::Client::SessionService service(properties, logger);

  // Get the created session id
  std::cout << "Session : " << service.getSession() << std::endl;
  std::string args;
  args.resize(10);
  args[0] = 'A';
  args[1] = 'r';
  args[2] = 'm';
  args[3] = '0';
  args[4] = '\0';
  args[5] = 'n';
  args[6] = 1;
  args[7] = 'K';
  args[8] = 0;
  args[9] = -128;

  ASSERT_EQ(args[0], 'A');

  // Create the handler
  auto handler = std::make_shared<EchoServiceHandler>(logger);

  // Submit a task
  auto tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("EchoService", args)}, handler);

  std::cout << "Sent : " << tasks[0] << std::endl;

  // Wait for task completion
  service.WaitResults();

  ASSERT_TRUE(!args.empty());
  ASSERT_TRUE(handler->received);
  ASSERT_FALSE(handler->is_error);

  std::cout << "Done" << std::endl;
}

TEST(testSDK, testAddInt) {
  std::cout << "Doing some computation!" << std::endl;
  // Load configuration from file and environment
  ArmoniK::Sdk::Common::Configuration config;
  config.add_json_configuration("appsettings.json").add_env_configuration();

  std::cout << "Endpoint : " << config.get("GrpcClient__Endpoint") << std::endl;
  if (config.get("Worker__Type").empty()) {
    config.set("Worker__Type", "End2EndTest");
  }

  // Create the task options
  ArmoniK::Sdk::Common::TaskOptions session_task_options(
      "libArmoniK.SDK.Worker.Test.so", config.get("WorkerLib__Version"), "End2EndTest", "AdditionService");
  session_task_options.max_retries = 1;

  // Create the properties
  ArmoniK::Sdk::Common::Properties properties{config, session_task_options};

  // Create the logger
  armonik::api::common::logger::Logger logger{armonik::api::common::logger::writer_console(),
                                              armonik::api::common::logger::formatter_plain(true)};

  // Create the session service
  ArmoniK::Sdk::Client::SessionService service(properties, logger);

  // Get the created session id
  std::cout << "Session : " << service.getSession() << std::endl;

  // ASSERT_EQ(args[0], "1");

  // Create the handler
  auto handler = std::make_shared<AddServiceHandler>();
  /*std::shared_ptr<ArmoniK::Sdk::Client::IServiceInvocationHandler> handler(
      (config.get("Worker__Type").c_str() == "PythonTestWorker")
          ? static_cast<ArmoniK::Sdk::Client::IServiceInvocationHandler *>(new PythonTestWorkerHandler)
          : (session_task_options.application_service == "EchoService")
          ? static_cast<ArmoniK::Sdk::Client::IServiceInvocationHandler *>(new EchoServiceHandler)
      : static_cast<ArmoniK::Sdk::Client::IServiceInvocationHandler *>(new AddServiceHandler));*/

  handler->is_int = true;

  std::string payload_val = StrSerialize<int>(52, 72);

  // Submit a task
  std::vector<std::string> task_ids;
  // for (auto &&arg : args) {
  auto tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_ints", payload_val)}, handler);
  service.WaitResults();

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  // auto result_payload =
  service.WaitResults();

  auto ans = handler->check_int_result(52, 72);
  // std::cout << "ans: " << ans << std::endl;
  ASSERT_EQ(handler->int_result, ans);
  ASSERT_TRUE(!handler->str.empty());

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_ints", StrSerialize<int>(23, 45))}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_int_result(23, 45);
  ASSERT_EQ(handler->int_result, ans);
  ASSERT_TRUE(!handler->str.empty());

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_ints", StrSerialize<int>(76, 34))}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_int_result(76, 34);
  ASSERT_EQ(handler->int_result, ans);
  ASSERT_TRUE(!handler->str.empty());

  handler->successCounter = 0;
  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_ints", StrSerialize<int>(82, 1)),
                          ArmoniK::Sdk::Common::TaskPayload("add_ints", StrSerialize<int>(4, 6))},
                         handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();
  ASSERT_EQ(handler->successCounter, 2);
  ASSERT_TRUE(!handler->str.empty());

  std::cout << "Done" << std::endl;
}

TEST(testSDK, testAddFloat) {
  std::cout << "Doing some computation!" << std::endl;
  // Load configuration from file and environment
  ArmoniK::Sdk::Common::Configuration config;
  config.add_json_configuration("appsettings.json").add_env_configuration();

  std::cout << "Endpoint : " << config.get("GrpcClient__Endpoint") << std::endl;
  if (config.get("Worker__Type").empty()) {
    config.set("Worker__Type", "End2EndTest");
  }

  // Create the task options
  ArmoniK::Sdk::Common::TaskOptions session_task_options(
      "libArmoniK.SDK.Worker.Test.so", config.get("WorkerLib__Version"), "End2EndTest", "AdditionService");
  session_task_options.max_retries = 1;

  // Create the properties
  ArmoniK::Sdk::Common::Properties properties{config, session_task_options};

  // Create the logger
  armonik::api::common::logger::Logger logger{armonik::api::common::logger::writer_console(),
                                              armonik::api::common::logger::formatter_plain(true)};

  // Create the session service
  ArmoniK::Sdk::Client::SessionService service(properties, logger);

  // Get the created session id
  std::cout << "Session : " << service.getSession() << std::endl;

  // Create the handler
  auto handler = std::make_shared<AddServiceHandler>();
  /*std::shared_ptr<ArmoniK::Sdk::Client::IServiceInvocationHandler> handler(
      (config.get("Worker__Type").c_str() == "PythonTestWorker")
          ? static_cast<ArmoniK::Sdk::Client::IServiceInvocationHandler *>(new PythonTestWorkerHandler)
          : (session_task_options.application_service == "EchoService")
          ? static_cast<ArmoniK::Sdk::Client::IServiceInvocationHandler *>(new EchoServiceHandler)
      : static_cast<ArmoniK::Sdk::Client::IServiceInvocationHandler *>(new AddServiceHandler));*/

  handler->is_int = false;
  // Submit a task
  std::vector<std::string> task_ids;

  auto tasks =
      service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_floats", StrSerialize<float>(32.3f, 21.2f))}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  // auto result_payload =
  service.WaitResults();

  auto ans = handler->check_float_result(32.3f, 21.2f);

  auto error = 0.0000001;

  EXPECT_NEAR(handler->float_result, ans, error);

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_floats", StrSerialize<float>(32.5f, 54.7f))}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_float_result(32.5f, 54.7f);
  EXPECT_NEAR(handler->float_result, ans, error);
  ASSERT_TRUE(!handler->str.empty());
  ASSERT_EQ(handler->str.size(), sizeof(float));

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_floats", StrSerialize<float>(2.9f, 1.07f))}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_float_result(2.9f, 1.07f);
  EXPECT_NEAR(handler->float_result, ans, error);
  ASSERT_TRUE(!handler->str.empty());
  ASSERT_EQ(handler->str.size(), sizeof(float));

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_floats", StrSerialize<float>(64.2f, 54.0f))}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_float_result(64.2f, 54.0f);
  EXPECT_NEAR(handler->float_result, ans, error);
  ASSERT_TRUE(!handler->str.empty());
  ASSERT_EQ(handler->str.size(), sizeof(float));

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_floats", StrSerialize<float>(7.6f, 8.5f))}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_float_result(7.6f, 8.5f);
  EXPECT_NEAR(handler->float_result, ans, error);
  ASSERT_TRUE(!handler->str.empty());
  ASSERT_EQ(handler->str.size(), sizeof(float));
  //}

  std::cout << "Done" << std::endl;
}

TEST(testSDK, testStressTest) {
  std::cout << "Doing some computation!" << std::endl;
  // Load configuration from file and environment
  ArmoniK::Sdk::Common::Configuration config;
  config.add_json_configuration("appsettings.json").add_env_configuration();

  std::cout << "Endpoint : " << config.get("GrpcClient__Endpoint") << std::endl;
  if (config.get("Worker__Type").empty()) {
    config.set("Worker__Type", "End2EndTest");
  }

  // Create the task options
  ArmoniK::Sdk::Common::TaskOptions session_task_options("libArmoniK.SDK.Worker.Test.so",
                                                         config.get("WorkerLib__Version"), "End2EndTest", "StressTest");
  session_task_options.max_retries = 1;

  // Create the properties
  ArmoniK::Sdk::Common::Properties properties{config, session_task_options};

  // Create the logger
  armonik::api::common::logger::Logger logger{armonik::api::common::logger::writer_console(),
                                              armonik::api::common::logger::formatter_plain(true)};

  // Create the session service
  ArmoniK::Sdk::Client::SessionService service(properties, logger);

  // Get the created session id
  std::cout << "Session : " << service.getSession() << std::endl;

  // Create the handler
  auto handler = std::make_shared<StressTestServiceHandler>(logger);

  const std::uint32_t nbTasks = 100, nbInputBytes = 64000, nbOutputBytes = 8, workloadTimeInMs = 1;

  const std::vector<double> input(nbInputBytes / sizeof(double),
                                  std::pow(42.0 * 8.0 / static_cast<double>(nbInputBytes), 1.0 / 3.0));
  std::cout << "StressTest default parameters:\n"
            << "number of tasks: " << nbTasks << '\n'
            << "number of input bytes: " << nbInputBytes << '\n'
            << "number of output bytes: " << nbOutputBytes << '\n'
            << "workload time in milliseconds: " << workloadTimeInMs << std::endl;

  auto payload = [nbInputBytes, nbOutputBytes, workloadTimeInMs, &input]() -> std::string {
    std::vector<char> outputVec(nbInputBytes + sizeof(nbOutputBytes) * 3, 0);
    auto beginPtr = outputVec.data();
    serialize_item(beginPtr, nbOutputBytes);

    serialize_item(beginPtr, workloadTimeInMs);

    serialize_item(beginPtr, nbInputBytes);

    serialize_item(beginPtr, input.data(), nbInputBytes / sizeof(double));
    return {outputVec.data(), outputVec.size()};
  }();
  const std::vector<ArmoniK::Sdk::Common::TaskPayload> tasks_payload(nbTasks, {"compute_workload", std::move(payload)});
  const auto tasks = service.Submit(tasks_payload, handler);

  ASSERT_FALSE(tasks.empty());
  ASSERT_EQ(tasks.size(), nbTasks);

  // Wait for task completion
  service.WaitResults();

  ASSERT_TRUE(handler->is_ok);
  ASSERT_EQ(handler->nb_output_bytes, nbOutputBytes);
  const auto result_out = compute_workload(input, nbOutputBytes, workloadTimeInMs);
  for (std::size_t i = 0; i < result_out.size(); ++i) {
    ASSERT_NEAR(handler->result.at(i), result_out.at(i), 0.0000001);
  }

  std::cout << "Done" << std::endl;
}

TEST(testSDK, testSegFault) {
  std::cout << "Testing SegFault in worker!" << std::endl;
  // Load configuration from file and environment
  ArmoniK::Sdk::Common::Configuration config;
  config.add_json_configuration("appsettings.json").add_env_configuration();

  std::cout << "Endpoint : " << config.get("GrpcClient__Endpoint") << std::endl;
  if (config.get("Worker__Type").empty()) {
    config.set("Worker__Type", "End2EndTest");
  }

  // Create the task options
  ArmoniK::Sdk::Common::TaskOptions session_task_options(
      "libArmoniK.SDK.Worker.Test.so", config.get("WorkerLib__Version"), "End2EndTest", "SegFaultService");
  session_task_options.max_retries = 1;

  // Create the properties
  ArmoniK::Sdk::Common::Properties properties{config, session_task_options};

  // Create the logger
  armonik::api::common::logger::Logger logger{armonik::api::common::logger::writer_console(),
                                              armonik::api::common::logger::formatter_plain(true)};

  // Create the session service
  ArmoniK::Sdk::Client::SessionService service(properties, logger);

  // Get the created session id
  std::cout << "Session : " << service.getSession() << std::endl;
  std::string args = "SegFault success";

  // Create the handler
  auto handler = std::make_shared<SegFaultServiceHandler>(logger);

  // Submit a task
  auto tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("SegFaultService", args)}, handler);

  std::cout << "Sent : " << tasks[0] << std::endl;

  // Wait for task completion
  service.WaitResults();

  ASSERT_TRUE(!args.empty());
  ASSERT_TRUE(handler->received);
  ASSERT_TRUE(handler->is_error);

  std::cout << "Done" << std::endl;
}
