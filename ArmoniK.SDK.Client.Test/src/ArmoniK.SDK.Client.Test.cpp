#include <cstring>
#include <gtest/gtest.h>
#include <iostream>

#include <armonik/common/logger/formatter.h>
#include <armonik/common/logger/logger.h>
#include <armonik/common/logger/writer.h>
#include <armonik/sdk/client/IServiceInvocationHandler.h>
#include <armonik/sdk/client/SessionService.h>
#include <armonik/sdk/common/Configuration.h>
#include <armonik/sdk/common/Properties.h>
#include <armonik/sdk/common/TaskPayload.h>

class PythonTestWorkerHandler : public ArmoniK::Sdk::Client::IServiceInvocationHandler {
public:
  void HandleResponse(const std::string &result_payload, const std::string &taskId) override {
    std::cout << "HANDLE RESPONSE : Received result of size " << result_payload.size() << " for taskId " << taskId
              << std::endl;
    auto tr = ArmoniK::Sdk::Common::TaskPayload::Deserialize(result_payload);
    std::cout << "Received : "
              << "\n Method name : " << tr.method_name << "\n Data dependencies : \n";
    for (auto &&dd : tr.data_dependencies) {
      std::cout << " - " << dd << '\n';
    }
    std::cout << " Args length : " << tr.arguments.size() << std::endl;
    std::cout << " Args data : " << tr.arguments.c_str() << std::endl;
  }
  void HandleError(const std::exception &e, const std::string &taskId) override {
    std::cerr << "HANDLE ERROR : Error for task id " << taskId << " : " << e.what() << std::endl;
  }
};

class AddServiceHandler : public ArmoniK::Sdk::Client::IServiceInvocationHandler {
public:
  void HandleResponse(const std::string &result_payload, const std::string &taskId) override {
    std::cout << "HANDLE RESPONSE : Received result of size " << result_payload.size() << " for taskId " << taskId
              << "\nContent : ";
    std::cout.write(result_payload.data(), result_payload.size()) << "\nRaw : ";
    std::cout << std::endl;

    for (char c : result_payload) {
      std::cout << static_cast<int>(c) << ' ';
    }
    std::cout << std::endl;

    str = result_payload.data();
    if (is_int) {
      std::memcpy(&int_result, result_payload.data(), sizeof(int32_t));
      std::cout << "HANDLE RESPONSE : Received result data value of " << int_result << std::endl;
    } else {
      std::memcpy(&float_result, result_payload.data(), sizeof(float));
      std::cout << "HANDLE RESPONSE : Received result data value of " << float_result << std::endl;
    }
  }
  void HandleError(const std::exception &e, const std::string &taskId) override {
    std::cerr << "HANDLE ERROR : Error for task id " << taskId << " : " << e.what() << std::endl;
  }

  size_t int_result = 0;
  float float_result = 0.0f;
  std::string str;
  bool is_int = true;

  int check_int_result(const int &a, const int &b) { return (a + b); }

  float check_float_result(const float &a, const float &b) { return (a + b); }
};

class EchoServiceHandler : public ArmoniK::Sdk::Client::IServiceInvocationHandler {
public:
  void HandleResponse(const std::string &result_payload, const std::string &taskId) override {
    std::cout << "HANDLE RESPONSE : Received result of size " << result_payload.size() << " for taskId " << taskId
              << "\nContent : ";
    std::cout.write(result_payload.data(), result_payload.size()) << "\nRaw : ";
    for (char c : result_payload) {
      std::cout << static_cast<int>(c) << ' ';
    }
    std::cout << std::endl;
  }
  void HandleError(const std::exception &e, const std::string &taskId) override {
    std::cerr << "HANDLE ERROR : Error for task id " << taskId << " : " << e.what() << std::endl;
  }
};

template <typename T> std::string StrSerialize(T a, T b) {
  std::string payload_val;

  payload_val.resize(sizeof(T) * 2);

  std::memcpy(payload_val.data(), &a, sizeof(T));
  std::memcpy(payload_val.data() + sizeof(T), &b, sizeof(T));

  return payload_val;
}

TEST(testSDK, testEcho) {
  std::cout << "Hello, World!" << std::endl;
  // Load configuration from file and environment
  ArmoniK::Sdk::Common::Configuration config;
  config.add_json_configuration("appsettings.json").add_env_configuration();

  std::cout << "Endpoint : " << config.get("Grpc__EndPoint") << std::endl;
  if (config.get("Worker__Type").empty()) {
    config.set("Worker__Type", "End2EndTest");
  }

  // Create the task options
  ArmoniK::Sdk::Common::TaskOptions session_task_options("libArmoniK.SDK.Worker.Test.so", "0.1.0", "End2EndTest",
                                                         "EchoService");

  // Create the properties
  ArmoniK::Sdk::Common::Properties properties{config, session_task_options};

  // Create the logger
  ArmoniK::Api::Common::logger::Logger logger{ArmoniK::Api::Common::logger::writer_console(),
                                              ArmoniK::Api::Common::logger::formatter_plain(true)};

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
  auto handler = std::make_shared<EchoServiceHandler>();

  // Submit a task
  auto tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("EchoService", args)}, handler);

  std::cout << "Sent : " << tasks[0] << std::endl;

  // Wait for task completion
  service.WaitResults();

  ASSERT_TRUE(!args.empty());

  std::cout << "Done" << std::endl;
}

TEST(testSDK, testAddInt) {
  std::cout << "Doing some computation!" << std::endl;
  // Load configuration from file and environment
  ArmoniK::Sdk::Common::Configuration config;
  config.add_json_configuration("appsettings.json").add_env_configuration();

  std::cout << "Endpoint : " << config.get("Grpc__EndPoint") << std::endl;
  if (config.get("Worker__Type").empty()) {
    config.set("Worker__Type", "End2EndTest");
  }

  // Create the task options
  ArmoniK::Sdk::Common::TaskOptions session_task_options("libArmoniK.SDK.Worker.Test.so", "0.1.0", "End2EndTest",
                                                         "AdditionService");

  // Create the properties
  ArmoniK::Sdk::Common::Properties properties{config, session_task_options};

  // Create the logger
  ArmoniK::Api::Common::logger::Logger logger{ArmoniK::Api::Common::logger::writer_console(),
                                              ArmoniK::Api::Common::logger::formatter_plain(true)};

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

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_ints", StrSerialize<int>(82, 1))}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_int_result(82, 1);
  ASSERT_EQ(handler->int_result, ans);
  ASSERT_TRUE(!handler->str.empty());

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_ints", StrSerialize<int>(4, 6))}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_int_result(4, 6);
  ASSERT_EQ(handler->int_result, ans);
  ASSERT_TRUE(!handler->str.empty());

  std::cout << "Done" << std::endl;
}

TEST(testSDK, testAddFloat) {
  std::cout << "Doing some computation!" << std::endl;
  // Load configuration from file and environment
  ArmoniK::Sdk::Common::Configuration config;
  config.add_json_configuration("appsettings.json").add_env_configuration();

  std::cout << "Endpoint : " << config.get("Grpc__EndPoint") << std::endl;
  if (config.get("Worker__Type").empty()) {
    config.set("Worker__Type", "End2EndTest");
  }

  // Create the task options
  ArmoniK::Sdk::Common::TaskOptions session_task_options("libArmoniK.SDK.Worker.Test.so", "0.1.0", "End2EndTest",
                                                         "AdditionService");

  // Create the properties
  ArmoniK::Sdk::Common::Properties properties{config, session_task_options};

  // Create the logger
  ArmoniK::Api::Common::logger::Logger logger{ArmoniK::Api::Common::logger::writer_console(),
                                              ArmoniK::Api::Common::logger::formatter_plain(true)};

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
      service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_floats", StrSerialize<float>(32.3, 21.2))}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  // auto result_payload =
  service.WaitResults();

  auto ans = handler->check_float_result(32.3, 21.2);

  auto error = 0.0000001;

  EXPECT_NEAR(handler->float_result, ans, error);

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_floats", StrSerialize<float>(32.5, 54.7))}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_float_result(32.5, 54.7);
  EXPECT_NEAR(handler->float_result, ans, error);
  ASSERT_TRUE(!handler->str.empty());
  ASSERT_EQ(handler->str.size(), sizeof(float));

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_floats", StrSerialize<float>(2.9, 1.07))}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_float_result(2.9, 1.07);
  EXPECT_NEAR(handler->float_result, ans, error);
  ASSERT_TRUE(!handler->str.empty());
  ASSERT_EQ(handler->str.size(), sizeof(float));

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_floats", StrSerialize<float>(64.2, 54.0))}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_float_result(64.2, 54.0);
  EXPECT_NEAR(handler->float_result, ans, error);
  ASSERT_TRUE(!handler->str.empty());
  ASSERT_EQ(handler->str.size(), sizeof(float));

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_floats", StrSerialize<float>(7.6, 8.5))}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_float_result(7.6, 8.5);
  EXPECT_NEAR(handler->float_result, ans, error);
  ASSERT_TRUE(!handler->str.empty());
  ASSERT_EQ(handler->str.size(), sizeof(float));
  //}

  std::cout << "Done" << std::endl;
}
