#pragma once

#include <map>
#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Worker {

/**
 * @brief Base class to implement a worker in ArmoniK
 * @note This class is useful for use with the ArmoniK.SDK.Worker library which uses the \link ArmoniKSDKInterface.h
 * ArmoniK SDK Interface \endlink
 */
class ServiceBase {
public:
  /**
   * @brief Method called when entering a session
   * @param session_id Null terminated id of the session
   * @return A user defined session context. May return null.
   */
  virtual void *enter_session(const char *session_id) {
    (void)session_id;
    return nullptr;
  }

  /**
   * @brief Method called when leaving a session
   * @param session_ctx User provided session context
   * @warning This function should free all resources contained in the session context
   */
  virtual void leave_session(void *session_ctx) { (void)session_ctx; }

  /**
   * @brief Convention-path entry point: called with named, already-resolved inputs.
   * Override this when using the convention execution path (TaskDefinition / SetDynamicLibrary).
   * @param session_ctx User provided session context
   * @param name Name of the called method
   * @param inputs Named inputs map (key → resolved string value)
   * @return Result string stored as a blob
   */
  virtual std::string call(void *session_ctx, const std::string &name,
                           const std::map<std::string, std::string> &inputs);

  /**
   * @brief Legacy entry point: called with the raw serialized payload.
   * Override this when using the legacy execution path (application_name / application_version).
   * The default implementation parses the payload as convention JSON and delegates to the map overload.
   * @param session_ctx User provided session context
   * @param name Name of the called method
   * @param input Raw payload — JSON for convention tasks, binary for legacy tasks
   * @return Result string stored as a blob
   */
  virtual std::string call(void *session_ctx, const std::string &name, const std::string &input);

  /**
   * @brief Service destructor
   */
  virtual ~ServiceBase() = default;
};
} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
