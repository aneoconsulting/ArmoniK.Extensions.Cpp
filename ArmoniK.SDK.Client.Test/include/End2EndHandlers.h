#pragma once

#include "armonik/sdk/client/IServiceInvocationHandler.h"

class PythonTestWorkerHandler final : public ArmoniK::Sdk::Client::IServiceInvocationHandler {
public:
  void HandleResponse(const std::string &result_payload, const std::string &taskId) override;
  void HandleError(const std::exception &e, const std::string &taskId) override;
};

class AddServiceHandler : public ArmoniK::Sdk::Client::IServiceInvocationHandler {
public:
  void HandleResponse(const std::string &result_payload, const std::string &taskId) override;
  void HandleError(const std::exception &e, const std::string &taskId) override;

  size_t int_result = 0;
  float float_result = 0.0f;
  std::string str;
  bool is_int = true;
  int successCounter = 0;

  int check_int_result(const int &a, const int &b) { return (a + b); }

  float check_float_result(const float &a, const float &b) { return (a + b); }
};

class EchoServiceHandler final : public ArmoniK::Sdk::Client::IServiceInvocationHandler {
public:
  void HandleResponse(const std::string &result_payload, const std::string &taskId) override;
  void HandleError(const std::exception &e, const std::string &taskId) override;
  bool received = false;
  bool is_error = false;
};
