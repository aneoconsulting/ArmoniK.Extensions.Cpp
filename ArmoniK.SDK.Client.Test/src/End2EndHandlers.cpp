#include <iostream>

#include "End2EndHandlers.h"
#include "armonik/sdk/common/TaskPayload.h"

void PythonTestWorkerHandler::HandleResponse(const std::string &result_payload, const std::string &taskId) {
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
void PythonTestWorkerHandler::HandleError(const std::exception &e, const std::string &taskId) {
  std::cerr << "HANDLE ERROR : Error for task id " << taskId << " : " << e.what() << std::endl;
}

void AddServiceHandler::HandleResponse(const std::string &result_payload, const std::string &taskId) {
  std::cout << "HANDLE RESPONSE : Received result of size " << result_payload.size() << " for taskId " << taskId
            << "\nContent : ";
  std::cout.write(result_payload.data(), result_payload.size()) << "\nRaw : ";
  std::cout << std::endl;

  for (char c : result_payload) {
    std::cout << static_cast<int>(c) << ' ';
  }
  std::cout << std::endl;

  str = result_payload;
  if (is_int) {
    std::memcpy(&int_result, result_payload.data(), sizeof(int32_t));
    std::cout << "HANDLE RESPONSE : Received result data value of " << int_result << std::endl;
  } else {
    std::memcpy(&float_result, result_payload.data(), sizeof(float));
    std::cout << "HANDLE RESPONSE : Received result data value of " << float_result << std::endl;
  }
  successCounter++;
}
void AddServiceHandler::HandleError(const std::exception &e, const std::string &taskId) {
  std::cerr << "HANDLE ERROR : Error for task id " << taskId << " : " << e.what() << std::endl;
}
void EchoServiceHandler::HandleResponse(const std::string &result_payload, const std::string &taskId) {
  std::cout << "HANDLE RESPONSE : Received result of size " << result_payload.size() << " for taskId " << taskId
            << "\nContent : ";
  std::cout.write(result_payload.data(), result_payload.size()) << "\nRaw : ";
  for (char c : result_payload) {
    std::cout << static_cast<int>(c) << ' ';
  }
  std::cout << std::endl;
  received = true;
  is_error = false;
}
void EchoServiceHandler::HandleError(const std::exception &e, const std::string &taskId) {
  std::cerr << "HANDLE ERROR : Error for task id " << taskId << " : " << e.what() << std::endl;
  received = true;
  is_error = true;
}
