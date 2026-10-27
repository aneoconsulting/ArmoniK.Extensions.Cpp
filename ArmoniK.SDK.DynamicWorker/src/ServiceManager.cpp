#include "ServiceManager.h"
#include "ContextIds.h"
#include <armonik/common/utils/string_view.h>
#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <armonik/sdk/common/internal/ConventionPayload.h>
#include <armonik/sdk/worker/TaskHandlerAgent.h>
#include <armonik/sdk/worker/internal/TaskApiHost.h>
#include <armonik/worker/Worker/ProcessStatus.h>
#include <stdexcept>
#include <utility>

namespace ArmoniK {
namespace Sdk {
namespace DynamicWorker {
namespace {
struct ArmonikContext {
  armonik::api::worker::TaskHandler &taskHandler;
  /** Result that armonik_call writes */
  std::string output_id;
  armonik::api::worker::ProcessStatus output;
  bool retry_requested = false;
  std::string retry_message;

  ArmonikContext(armonik::api::worker::TaskHandler &taskHandler, std::string output_id)
      : taskHandler(taskHandler), output_id(std::move(output_id)) {}
};

/**
 * @brief Task status from the status of a library call: a retry throws, so that ArmoniK retries the task
 */
armonik::api::worker::ProcessStatus Finish(ArmonikContext &callContext, armonik_status_t status) {
  if (callContext.retry_requested) {
    throw std::runtime_error(callContext.retry_message);
  }
  if (status != ARMONIK_STATUS_OK) {
    if (callContext.output.ok()) {
      callContext.output.set_error("Unknown error in worker, check logs.");
    }
  } else {
    callContext.output.set_ok();
  }
  return callContext.output;
}
} // namespace

ServiceManager::ServiceManager(ArmoniKFunctionPointers functionsPointers, ServiceId serviceId)
    : serviceId(std::move(serviceId)), functionPointers(functionsPointers) {
  service_context = this->functionPointers.create_service(this->serviceId.service_namespace.c_str(),
                                                          this->serviceId.service_name.c_str());
}
ServiceManager::~ServiceManager() { clear(); }
ServiceManager &ServiceManager::UseSession(const std::string &sessionId) & {
  if (sessionId != current_session) {
    if (!current_session.empty()) {
      functionPointers.leave_session(service_context, session_context);
    }
    current_session = sessionId;
    session_context = functionPointers.enter_session(service_context, sessionId.c_str());
  }
  return *this;
}
armonik::api::worker::ProcessStatus ServiceManager::Execute(armonik::api::worker::TaskHandler &taskHandler,
                                                            const std::string &method_name,
                                                            const std::string &method_arguments) {
  const auto &expected = taskHandler.getExpectedResults();
  if (expected.empty()) {
    throw ArmoniK::Sdk::Common::ArmoniKSdkException("Task has no expected result");
  }
  ArmonikContext callContext(taskHandler, expected[0]);
  if (current_session.empty()) {
    throw ArmoniK::Sdk::Common::ArmoniKSdkException("Session is not initialized");
  }
  auto status = functionPointers.call(&callContext, service_context, session_context, method_name.c_str(),
                                      method_arguments.data(), method_arguments.size(), ServiceManager::UploadResult);
  return Finish(callContext, status);
}

armonik::api::worker::ProcessStatus ServiceManager::Execute(armonik::api::worker::TaskHandler &taskHandler,
                                                            const ArmoniK::Sdk::Common::ConventionTask &task) {
  const auto &outputs = task.outputs;
  if (current_session.empty()) {
    throw ArmoniK::Sdk::Common::ArmoniKSdkException("Session is not initialized");
  }

  if (functionPointers.process == nullptr) {
    // Library built before armonik_process: armonik_call returns a single output
    if (outputs.size() != 1) {
      throw ArmoniK::Sdk::Common::ArmoniKSdkException(
          "The task has " + std::to_string(outputs.size()) +
          " outputs, but the library does not export armonik_process: rebuild it with the current "
          "ArmoniK.SDK.Worker");
    }
    ArmoniK::Sdk::Common::ConventionPayload payload;
    payload.inputs = task.inputs;
    payload.outputs = outputs;
    const auto arguments = payload.Serialize();
    ArmonikContext callContext(taskHandler, outputs.begin()->second);
    auto status = functionPointers.call(&callContext, service_context, session_context, task.method_name.c_str(),
                                        arguments.data(), arguments.size(), ServiceManager::UploadResult);
    return Finish(callContext, status);
  }

  ArmoniK::Sdk::Worker::TaskHandlerAgent agent(taskHandler);
  ArmoniK::Sdk::Worker::Internal::TaskApiHost host(agent, task.inputs, task.input_ids, outputs);
  ArmonikContext callContext(taskHandler, std::string());
  auto status = functionPointers.process(&callContext, &host.task(), service_context, session_context,
                                         task.method_name.c_str(), ServiceManager::ReportFailure);
  auto result = Finish(callContext, status);
  // The library may define its own armonik_process: check its outputs here
  if (result.ok()) {
    host.finish();
  }
  return result;
}

void ServiceManager::UploadResult(void *opaque_context, armonik_status_t status, const char *data, size_t data_size) {
  auto context = static_cast<ArmonikContext *>(opaque_context);
  if (status != ARMONIK_STATUS_OK) {
    ReportFailure(opaque_context, status, data, data_size);
    return;
  }
  try {
    context->taskHandler.send_result(context->output_id, armonik::api::string_view(data, data_size)).get();
    context->output.set_ok();
  } catch (const std::exception &e) {
    // The result could not reach the agent: a transient failure
    context->retry_requested = true;
    context->retry_message = std::string("Failed to send the result: ") + e.what();
  }
}

void ServiceManager::ReportFailure(void *opaque_context, armonik_status_t status, const char *data, size_t data_size) {
  auto context = static_cast<ArmonikContext *>(opaque_context);
  if (status == ARMONIK_STATUS_RETRY) {
    context->retry_requested = true;
    context->retry_message = std::string(data, data_size);
  } else if (status != ARMONIK_STATUS_OK) {
    context->output.set_error(std::string(data, data_size));
  }
}
bool ServiceManager::matches(const ServiceId &other) { return other == serviceId; }
void ServiceManager::clear() {
  if (serviceId.empty()) {
    return;
  }
  if (!current_session.empty()) {
    functionPointers.leave_session(service_context, session_context);
    current_session.clear();
  }
  functionPointers.destroy_service(service_context);
  serviceId.clear();
}
} // namespace DynamicWorker
} // namespace Sdk
} // namespace ArmoniK
