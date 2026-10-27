#include "armonik/sdk/worker/ArmoniKSDKInterface.h"
#include "armonik/sdk/common/ArmoniKSdkException.h"
#include "armonik/sdk/common/internal/ConventionPayload.h"
#include "armonik/sdk/worker/ServiceBase.h"
#include "armonik/sdk/worker/TaskContext.h"
#include "armonik/sdk/worker/internal/TaskOptionsJson.h"
#include <cstring>
#include <stdexcept>

// Implementation details, hidden from the documentation
/// @cond
namespace {

using ArmoniK::Sdk::Worker::TaskAgent;

armonik_string_t String(const std::string &s) { return {s.data(), s.size()}; }

/**
 * @brief Library side of armonik_process: the operations of the armonik_task_t given by the worker
 */
class LibraryTaskAgent final : public TaskAgent {
public:
  explicit LibraryTaskAgent(const armonik_task_t &task)
      : task_(task), session_id_(task.session_id.data, task.session_id.size),
        task_id_(task.task_id.data, task.task_id.size),
        options_(ArmoniK::Sdk::Worker::Internal::TaskOptionsFromJson(task.task_options.data, task.task_options.size)) {}

  const std::string &session_id() const override { return session_id_; }
  const std::string &task_id() const override { return task_id_; }
  const ArmoniK::Sdk::Common::TaskOptions &task_options() const override { return options_; }

  std::vector<std::string> create_results_metadata(const std::vector<std::string> &names) override {
    std::vector<armonik_string_t> c_names;
    for (const auto &name : names) {
      c_names.push_back(String(name));
    }
    Sink sink;
    Check(task_.create_results_metadata(task_.agent, c_names.data(), c_names.size(), &sink, &Sink::OnValue,
                                        &Sink::OnError),
          sink);
    return std::move(sink.values);
  }

  void send_results(const std::vector<ResultData> &results) override {
    std::vector<armonik_result_data_t> c_results;
    for (const auto &result : results) {
      c_results.push_back({String(result.id), {result.data, result.size}});
    }
    Sink sink;
    Check(task_.send_results(task_.agent, c_results.data(), c_results.size(), &sink, &Sink::OnError), sink);
  }

  std::vector<std::string> submit_tasks(const ArmoniK::Sdk::Common::TaskOptions &task_options,
                                        const std::vector<TaskRequest> &tasks) override {
    const auto options = ArmoniK::Sdk::Worker::Internal::TaskOptionsToJson(task_options);
    std::vector<std::vector<armonik_string_t>> dependencies(tasks.size());
    std::vector<std::vector<armonik_string_t>> outputs(tasks.size());
    std::vector<armonik_task_request_t> c_tasks(tasks.size());
    for (std::size_t i = 0; i < tasks.size(); ++i) {
      for (const auto &id : tasks[i].data_dependencies) {
        dependencies[i].push_back(String(id));
      }
      for (const auto &id : tasks[i].expected_outputs) {
        outputs[i].push_back(String(id));
      }
      c_tasks[i] = {String(tasks[i].payload), dependencies[i].data(), dependencies[i].size(), outputs[i].data(),
                    outputs[i].size()};
    }
    Sink sink;
    Check(task_.submit_tasks(task_.agent, String(options), c_tasks.data(), c_tasks.size(), &sink, &Sink::OnValue,
                             &Sink::OnError),
          sink);
    return std::move(sink.values);
  }

private:
  struct Sink {
    std::vector<std::string> values;
    std::string error;

    static void OnValue(void *sink, const char *data, size_t size) {
      static_cast<Sink *>(sink)->values.emplace_back(data, size);
    }
    static void OnError(void *sink, const char *data, size_t size) {
      static_cast<Sink *>(sink)->error.assign(data, size);
    }
  };

  /**
   * @brief Turns a failed operation back into the exception the worker reported: a failure, or a transient failure
   */
  static void Check(armonik_status_t status, const Sink &sink) {
    if (status == ARMONIK_STATUS_ERROR) {
      throw ArmoniK::Sdk::Common::ArmoniKSdkException(sink.error);
    }
    if (status != ARMONIK_STATUS_OK) {
      throw std::runtime_error(sink.error);
    }
  }

  const armonik_task_t &task_;
  std::string session_id_;
  std::string task_id_;
  ArmoniK::Sdk::Common::TaskOptions options_;
};

/**
 * @brief Result of an armonik_call made by armonik_process_default
 */
struct CallResult {
  armonik_status_t status = ARMONIK_STATUS_ERROR;
  std::string data;

  static void Collect(void *context, armonik_status_t status, const char *data, size_t size) {
    auto result = static_cast<CallResult *>(context);
    result->status = status;
    result->data.assign(data, size);
  }
};

} // namespace
/// @endcond

extern "C" {

void armonik_destroy_service_default(void *p) {
  if (p) {
    delete static_cast<ArmoniK::Sdk::Worker::ServiceBase *>(p);
  }
}

#ifdef __linux__
__attribute__((weak, alias("armonik_destroy_service_default")))
#endif
  void armonik_destroy_service(void *p);

void *armonik_enter_session_default(void *service_context, const char *session_id) {
  return static_cast<ArmoniK::Sdk::Worker::ServiceBase *>(service_context)->enter_session(session_id);
}

#ifdef __linux__
__attribute__((weak, alias("armonik_enter_session_default")))
#endif
void *
armonik_enter_session(void *service_context, const char *session_id);

void armonik_leave_session_default(void *service_context, void *session_context) {
  static_cast<ArmoniK::Sdk::Worker::ServiceBase *>(service_context)->leave_session(session_context);
}

#ifdef __linux__
__attribute__((weak, alias("armonik_leave_session_default")))
#endif
  void armonik_leave_session(void *service_context,
                                                                                         void *session_context);

/**
 * \inherit armonik_call
 */
armonik_status_t armonik_call_default(void *armonik_context, void *service_context, void *session_context,
                                      const char *function_name, const char *input, size_t input_size,
                                      armonik_callback_t callback) {
  try {
    auto output = static_cast<ArmoniK::Sdk::Worker::ServiceBase *>(service_context)
                      ->call(session_context, std::string(function_name), std::string(input, input_size));
    callback(armonik_context, ARMONIK_STATUS_OK, output.data(), output.size());
    return ARMONIK_STATUS_OK;
  } catch (const ArmoniK::Sdk::Common::ArmoniKSdkException &e) {
    auto msg = e.what();
    callback(armonik_context, ARMONIK_STATUS_ERROR, msg, std::strlen(msg));
    return ARMONIK_STATUS_ERROR;
  } catch (const std::exception &e) {
    auto msg = e.what();
    callback(armonik_context, ARMONIK_STATUS_RETRY, msg, std::strlen(msg));
    return ARMONIK_STATUS_RETRY;
  } catch (...) {
    // No exception crosses the C interface
    static const char msg[] = "Unknown exception in the library";
    callback(armonik_context, ARMONIK_STATUS_RETRY, msg, sizeof(msg) - 1);
    return ARMONIK_STATUS_RETRY;
  }
}

#ifdef __linux__
__attribute__((weak, alias("armonik_call_default")))
#endif
armonik_status_t
armonik_call(void *armonik_context, void *service_context, void *session_context, const char *function_name,
             const char *input, size_t input_size, armonik_callback_t callback);

/**
 * \inherit armonik_process
 */
armonik_status_t armonik_process_default(void *armonik_context, const armonik_task_t *task, void *service_context,
                                         void *session_context, const char *function_name,
                                         armonik_callback_t callback) {
  try {
    if (task == nullptr || task->struct_size < sizeof(armonik_task_t)) {
      throw ArmoniK::Sdk::Common::ArmoniKSdkException("The worker passed an incompatible armonik_task_t");
    }
    LibraryTaskAgent agent(*task);
    std::map<std::string, std::string> input_values;
    std::map<std::string, std::string> input_ids;
    std::map<std::string, std::string> outputs;
    for (size_t i = 0; i < task->inputs_count; ++i) {
      const auto &input = task->inputs[i];
      std::string name(input.name.data, input.name.size);
      if (input.id.size != 0) {
        input_ids.emplace(name, std::string(input.id.data, input.id.size));
      }
      input_values.emplace(std::move(name), std::string(input.data.data, input.data.size));
    }
    for (size_t i = 0; i < task->outputs_count; ++i) {
      const auto &output = task->outputs[i];
      outputs.emplace(std::string(output.name.data, output.name.size), std::string(output.id.data, output.id.size));
    }

#ifdef __linux__
    // A library that defines its own armonik_call keeps it: its service context may not be a ServiceBase.
    // Read through a volatile so that the compiler does not fold the comparison of the weak alias.
    armonik_call_t volatile call = &armonik_call;
    if (call != &armonik_call_default) {
      if (outputs.size() != 1) {
        throw ArmoniK::Sdk::Common::ArmoniKSdkException(
            "The task has " + std::to_string(outputs.size()) +
            " outputs, but the library defines armonik_call and not armonik_process");
      }
      ArmoniK::Sdk::Common::ConventionPayload payload;
      payload.inputs = std::move(input_values);
      payload.outputs = outputs;
      const auto arguments = payload.Serialize();
      CallResult result;
      const auto status = call(&result, service_context, session_context, function_name, arguments.data(),
                               arguments.size(), &CallResult::Collect);
      // A call that does not give its result through the callback failed
      if (status != ARMONIK_STATUS_OK || result.status != ARMONIK_STATUS_OK) {
        const auto failed = status != ARMONIK_STATUS_OK ? status : result.status;
        callback(armonik_context, failed, result.data.data(), result.data.size());
        return failed;
      }
      agent.send_results({{outputs.begin()->second, result.data.data(), result.data.size()}});
      return ARMONIK_STATUS_OK;
    }
#endif

    ArmoniK::Sdk::Worker::TaskContext context(agent, std::move(input_values), input_ids, outputs,
                                              ArmoniK::Sdk::Worker::TaskContext::DefaultCall::Payload);
    static_cast<ArmoniK::Sdk::Worker::ServiceBase *>(service_context)
        ->process(session_context, std::string(function_name), context);
    context.finish();
    return ARMONIK_STATUS_OK;
  } catch (const ArmoniK::Sdk::Common::ArmoniKSdkException &e) {
    auto msg = e.what();
    callback(armonik_context, ARMONIK_STATUS_ERROR, msg, std::strlen(msg));
    return ARMONIK_STATUS_ERROR;
  } catch (const std::exception &e) {
    auto msg = e.what();
    callback(armonik_context, ARMONIK_STATUS_RETRY, msg, std::strlen(msg));
    return ARMONIK_STATUS_RETRY;
  } catch (...) {
    // No exception crosses the C interface
    static const char msg[] = "Unknown exception in the library";
    callback(armonik_context, ARMONIK_STATUS_RETRY, msg, sizeof(msg) - 1);
    return ARMONIK_STATUS_RETRY;
  }
}

#ifdef __linux__
__attribute__((weak, alias("armonik_process_default")))
#endif
armonik_status_t
armonik_process(void *armonik_context, const armonik_task_t *task, void *service_context, void *session_context,
                const char *function_name, armonik_callback_t callback);
}
