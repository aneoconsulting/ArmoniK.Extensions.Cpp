#include <gtest/gtest.h>
#include <iostream>

#include <armonik/sdk/client/IServiceInvocationHandler.h>
#include <armonik/sdk/client/SessionService.h>
#include <armonik/sdk/common/Configuration.h>
#include <armonik/sdk/common/Properties.h>
#include <armonik/sdk/common/TaskPayload.h>

class PythonTestWorkerHandler : public ArmoniK::Sdk::Client::IServiceInvocationHandler {
public:
  void HandleResponse(const std::string &result_payload, const std::string &taskId) override {
    std::cout << "HANDLE RESPONSE : Received result of size " << result_payload.size() << std::endl;
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
    std::cout << "HANDLE RESPONSE : Received result of size " << result_payload.size() << std::endl;
    std::cout << "HANDLE RESPONSE : Received result data string of " << result_payload.data() << std::endl;

    str = result_payload.data();
    if (is_int) {
      int_result = std::stoi(result_payload.data());
      std::cout << "HANDLE RESPONSE : Received result data value of " << std::stoi(result_payload.data()) << std::endl;
    } else {
      float_result = std::stof(result_payload.data());
      std::cout << "HANDLE RESPONSE : Received result data value of " << std::stof(result_payload.data()) << std::endl;
    }
  }
  void HandleError(const std::exception &e, const std::string &taskId) override {
    std::cerr << "HANDLE ERROR : Error for task id " << taskId << " : " << e.what() << std::endl;
  }

  size_t int_result;
  float float_result;
  std::string str;
  bool is_int = true;

  int check_int_result(const int &val) { return (val + val + sizeof(int)); }

  float check_float_result(const float &val) { return (val + val + sizeof(float)); }
};

class EchoServiceHandler : public ArmoniK::Sdk::Client::IServiceInvocationHandler {
public:
  void HandleResponse(const std::string &result_payload, const std::string &taskId) override {
    std::cout << "HANDLE RESPONSE : Received result of size " << result_payload.size() << std::endl;
  }
  void HandleError(const std::exception &e, const std::string &taskId) override {
    std::cerr << "HANDLE ERROR : Error for task id " << taskId << " : " << e.what() << std::endl;
  }
};

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
  ArmoniK::Sdk::Common::TaskOptions session_task_options("libArmoniK.SDK.DLLWorker.Test.so", "0.1.0", "End2EndTest",
                                                         "EchoService");

  // Create the properties
  ArmoniK::Sdk::Common::Properties properties(config, session_task_options);

  // Create the session service
  ArmoniK::Sdk::Client::SessionService service(properties);

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
  // auto handler = std::make_shared<PythonTestWorkerHandler>();
  std::shared_ptr<ArmoniK::Sdk::Client::IServiceInvocationHandler> handler(
      (config.get("Worker__Type") == "PythonTestWorker")
          ? static_cast<ArmoniK::Sdk::Client::IServiceInvocationHandler *>(new PythonTestWorkerHandler)
          : static_cast<ArmoniK::Sdk::Client::IServiceInvocationHandler *>(new EchoServiceHandler));

  // Submit a task
  auto tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("EchoService", args)}, handler);

  std::cout << "Sent : " << tasks[0] << std::endl;

  // Wait for task completion
  // service.WaitResults();

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
  ArmoniK::Sdk::Common::TaskOptions session_task_options("libArmoniK.SDK.DLLWorker.Test.so", "0.1.0", "End2EndTest",
                                                         "AdditionService");

  // Create the properties
  ArmoniK::Sdk::Common::Properties properties(config, session_task_options);

  // Create the session service
  ArmoniK::Sdk::Client::SessionService service(properties);

  // Get the created session id
  std::cout << "Session : " << service.getSession() << std::endl;

  std::vector<std::string> args = {"5", "2", "6", "90", "455"};

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
  // Submit a task
  std::vector<std::string> task_ids;
  size_t n = 0;
  // for (auto &&arg : args) {
  auto tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_ints", args[n])}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  // auto result_payload =
  service.WaitResults();

  auto ans = handler->check_int_result(std::stoi(args[n++]));
  // std::cout << "ans: " << ans << std::endl;
  ASSERT_EQ(handler->int_result, ans);
  ASSERT_TRUE(!handler->str.empty());

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_ints", args[n])}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_int_result(std::stoi(args[n++]));
  ASSERT_EQ(handler->int_result, ans);
  ASSERT_TRUE(!handler->str.empty());

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_ints", args[n])}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_int_result(std::stoi(args[n++]));
  ASSERT_EQ(handler->int_result, ans);
  ASSERT_TRUE(!handler->str.empty());

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_ints", args[n])}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_int_result(std::stoi(args[n++]));
  ASSERT_EQ(handler->int_result, ans);
  ASSERT_TRUE(!handler->str.empty());

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_ints", args[n])}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_int_result(std::stoi(args[n]));
  ASSERT_EQ(handler->int_result, ans);
  ASSERT_TRUE(!handler->str.empty());
  //}

  ASSERT_EQ((2 + 2), 4);

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
  ArmoniK::Sdk::Common::TaskOptions session_task_options("libArmoniK.SDK.DLLWorker.Test.so", "0.1.0", "End2EndTest",
                                                         "AdditionService");

  // Create the properties
  ArmoniK::Sdk::Common::Properties properties(config, session_task_options);

  // Create the session service
  ArmoniK::Sdk::Client::SessionService service(properties);

  // Get the created session id
  std::cout << "Session : " << service.getSession() << std::endl;

  std::vector<std::string> args = {"5.12", "241.23", "0.6", "1.290", "45.75"};

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
  size_t n = 0;
  // for (auto &&arg : args) {
  auto tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_floats", args[n])}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  // auto result_payload =
  service.WaitResults();

  auto ans = handler->check_float_result(std::stof(args[n++]));

  auto error = 0.000000001;
  // std::cout << "ans: " << ans << std::endl;
  EXPECT_NEAR(handler->float_result, ans, error);
  ASSERT_TRUE(!handler->str.empty());

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_floats", args[n])}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_float_result(std::stof(args[n++]));
  EXPECT_NEAR(handler->float_result, ans, error);
  ASSERT_TRUE(!handler->str.empty());

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_floats", args[n])}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_float_result(std::stof(args[n++]));
  EXPECT_NEAR(handler->float_result, ans, error);
  ASSERT_TRUE(!handler->str.empty());

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_floats", args[n])}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_float_result(std::stof(args[n++]));
  EXPECT_NEAR(handler->float_result, ans, error);
  ASSERT_TRUE(!handler->str.empty());

  tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("add_floats", args[n])}, handler);

  ASSERT_FALSE(tasks.empty());

  // Wait for task completion
  service.WaitResults();

  ans = handler->check_float_result(std::stof(args[n]));
  EXPECT_NEAR(handler->float_result, ans, error);
  ASSERT_TRUE(!handler->str.empty());
  //}

  ASSERT_EQ((2 + 2), 4);

  std::cout << "Done" << std::endl;
}
