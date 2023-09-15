#pragma once

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
   * @brief Method called when a method is called
   * @param session_ctx User provided session context
   * @param name Name of the called method
   * @param input Arguments of the called method, in a serialized form
   * @return Called method return value in a serialized form
   */
  virtual std::string call(void *session_ctx, const std::string &name, const std::string &input) = 0;

  /**
   * @brief Service destructor
   */
  virtual ~ServiceBase() = default;
};
} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
