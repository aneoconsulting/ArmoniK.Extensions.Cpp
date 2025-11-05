#include <iostream>
#include <armonik/common/logger/formatter.h>
#include <armonik/common/logger/logger.h>
#include <armonik/common/logger/writer.h>
#include <armonik/sdk/client/IServiceInvocationHandler.h>
#include <armonik/sdk/client/SessionService.h>
#include <armonik/sdk/common/Configuration.h>
#include <armonik/sdk/common/Properties.h>
#include <armonik/sdk/common/TaskPayload.h>
#include "HelloServiceHandler.h"

int main() {
    std::cout << "Starting Hello World ArmoniK Client..." << std::endl;

    // Load configuration from file and environment
    ArmoniK::Sdk::Common::Configuration config;
    config.add_json_configuration("appsettings.json").add_env_configuration();
    
    std::cout << "\nEndpoint: " << config.get("GrpcClient__Endpoint") << std::endl;

    // Setup task options for Hello Service
    ArmoniK::Sdk::Common::TaskOptions session_task_options("libArmoniK.SDK.Worker.Examples.Hello.so", 
                                                   config.get("WorkerLib__Version"), 
                                                   "Examples", "HelloService",
                                                   config.get("PartitionId"));
    session_task_options.max_retries = 1;

    // Create properties for the service
    ArmoniK::Sdk::Common::Properties properties{config, session_task_options};

    // Setup logging
    armonik::api::common::logger::Logger logger{
        armonik::api::common::logger::writer_console(),
        armonik::api::common::logger::formatter_plain(true)};

    // Initialize session service
    ArmoniK::Sdk::Client::SessionService service(properties, logger);

    // Create a session and prepare a simple "Hello World" task
    std::cout << "Session ID: " << service.getSession() << std::endl;
    std::string inputData = "Hello,"; // The input for our task

    // Create a task payload
    ArmoniK::Sdk::Common::TaskPayload task_payload("HelloService", inputData);

    // Create the handler
    auto handler = std::make_shared<HelloServiceHandler>(logger);
      
    // Submit a task
    auto tasks = service.Submit({ArmoniK::Sdk::Common::TaskPayload("HelloService", inputData)}, handler);

    std::cout << "Task Submitted: " << tasks[0] << std::endl;

    // Wait for results
    service.WaitResults();

    std::cout << "Task Processing Complete." << std::endl;

    return 0;
}
