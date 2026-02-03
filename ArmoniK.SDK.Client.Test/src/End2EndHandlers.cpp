#include <iostream>

#include "End2EndHandlers.h"
#include "armonik/sdk/common/TaskPayload.h"

void PythonTestWorkerHandler::HandleResponse(const std::string &result_payload, const std::string &taskId) {
  std::cout << "HANDLE RESPONSE : Received result of size " << result_payload.size() << " for taskId " << taskId
            << std::endl;
  auto tr = ArmoniK::Sdk::Common::TaskPayload::Deserialize(result_payload);
  std::cout << "Received : " << "\n Method name : " << tr.method_name << "\n Data dependencies : \n";
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
  std::lock_guard<std::mutex> lock(mutex);

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
  std::lock_guard<std::mutex> lock(mutex);

  std::stringstream ss;
  ss << "HANDLE RESPONSE : Received result of size " << result_payload.size() << " for taskId " << taskId
     << "\nContent : ";
  ss.write(result_payload.data(), result_payload.size()) << "\nRaw : ";
  for (char c : result_payload) {
    ss << static_cast<int>(c) << ' ';
  }
  ss << std::endl;
  logger.debug(ss.str());
  received = true;
  is_error = false;
}
void EchoServiceHandler::HandleError(const std::exception &e, const std::string &taskId) {
  std::lock_guard<std::mutex> lock(mutex);

  std::stringstream ss;
  ss << "HANDLE ERROR : Error for task id " << taskId << " : " << e.what() << std::endl;
  logger.debug(ss.str());
  received = true;
  is_error = true;
}
EchoServiceHandler::EchoServiceHandler(armonik::api::common::logger::Logger &logger) : logger(logger.local()) {}

StressTestServiceHandler::StressTestServiceHandler(armonik::api::common::logger::Logger &logger)
    : logger(logger.local()) {}
void StressTestServiceHandler::HandleResponse(const std::string &result_payload, const std::string &taskId) {
  std::lock_guard<std::mutex> lock(mutex);

  std::stringstream ss;
  nb_output_bytes = result_payload.size();
  if (nb_output_bytes == 0) {
    is_ok = false;
  }
  result.resize(result_payload.size() / sizeof(double), 0.0);
  std::memcpy(result.data(), result_payload.data(), result_payload.size());
  ss << "Handle response: received result of size: " << result_payload.size() << " for taskId " << taskId << "\nRaw: ";
  ss.write(result_payload.data(), result_payload.size()) << "\nContent: ";
  std::for_each(result.cbegin(), result.cend(), [&ss](const auto &v) { ss << v << ' '; });
  ss << std::endl;
  logger.debug(ss.str());
}
void StressTestServiceHandler::HandleError(const std::exception &e, const std::string &taskId) {
  std::lock_guard<std::mutex> lock(mutex);

  is_ok = false;
  std::stringstream ss;
  ss << "Handle ERROR: Error for task id " << taskId << ": " << e.what() << '\n';
  logger.debug(ss.str());
}

void SegFaultServiceHandler::HandleResponse(const std::string &result_payload, const std::string &taskId) {
  std::lock_guard<std::mutex> lock(mutex);

  std::stringstream ss;
  ss << "HANDLE RESPONSE : Received result of size " << result_payload.size() << " for taskId " << taskId
     << "\nContent : ";
  ss.write(result_payload.data(), result_payload.size()) << "\nRaw : ";
  for (char c : result_payload) {
    ss << static_cast<int>(c) << ' ';
  }
  ss << std::endl;
  logger.debug(ss.str());
  received = true;
  is_error = false;
}
void SegFaultServiceHandler::HandleError(const std::exception &e, const std::string &taskId) {
  std::lock_guard<std::mutex> lock(mutex);

  std::stringstream ss;
  ss << "HANDLE ERROR : Error for task id " << taskId << " : " << e.what() << std::endl;
  logger.debug(ss.str());
  received = true;
  is_error = true;
}
SegFaultServiceHandler::SegFaultServiceHandler(armonik::api::common::logger::Logger &logger) : logger(logger.local()) {}

void SleepServiceHandler::HandleResponse(const std::string &result_payload, const std::string &taskId) {
  std::lock_guard<std::mutex> lock(mutex);

  std::stringstream ss;
  ss << "HANDLE RESPONSE : Received result of size " << result_payload.size() << " for taskId " << taskId
     << "\nContent : ";
  ss.write(result_payload.data(), result_payload.size()) << "\nRaw : ";
  for (char c : result_payload) {
    ss << static_cast<int>(c) << ' ';
  }
  ss << std::endl;
  logger.log(armonik::api::common::logger::Level::Debug, ss.str());
  received = true;
  is_error = false;
  received_count++;
}
void SleepServiceHandler::HandleError(const std::exception &e, const std::string &taskId) {
  std::lock_guard<std::mutex> lock(mutex);

  std::stringstream ss;
  ss << "HANDLE ERROR : Error for task id " << taskId << " : " << e.what() << std::endl;
  logger.log(armonik::api::common::logger::Level::Debug, ss.str());
  received = true;
  is_error = true;
}
SleepServiceHandler::SleepServiceHandler(armonik::api::common::logger::Logger &logger) : logger(logger.local()) {}

CountServiceHandler::CountServiceHandler(armonik::api::common::logger::Logger &logger) : logger(logger.local()) {}
void CountServiceHandler::HandleResponse(const std::string &result_payload, const std::string &taskId) {
  std::lock_guard<std::mutex> lock(mutex);

  std::stringstream ss;
  ss << "HANDLE RESPONSE : Received result of size " << result_payload.size() << " for taskId " << taskId;
  success++;
  logger.log(armonik::api::common::logger::Level::Debug, ss.str(),
             {{"success", std::to_string(success)}, {"failure", std::to_string(failure)}});
}
void CountServiceHandler::HandleError(const std::exception &e, const std::string &taskId) {
  std::lock_guard<std::mutex> lock(mutex);

  std::stringstream ss;
  ss << "HANDLE ERROR : Error for task id " << taskId << " : " << e.what();
  failure++;
  logger.log(armonik::api::common::logger::Level::Debug, ss.str(),
             {{"success", std::to_string(success)}, {"failure", std::to_string(failure)}});
}
