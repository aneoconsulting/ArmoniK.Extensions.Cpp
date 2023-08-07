#ifndef ARMONIK_SDK_ISERVICEINVOCATIONHANDLER_H
#define ARMONIK_SDK_ISERVICEINVOCATIONHANDLER_H

#include <string>

namespace SDK_CLIENT_NAMESPACE {

/**
 * @brief Task result handler interface class
 */
class IServiceInvocationHandler {
public:
  /**
   * @brief Callback function called when a tasks succeeds
   * @param result_payload Task result
   * @param taskId Task Id
   */
  virtual void HandleResponse(const std::string &result_payload, const std::string &taskId) = 0;

  /**
   * @brief Callback function called when a tasks fails
   * @param e Risen error
   * @param taskId Task Id
   */
  virtual void HandleError(const std::exception &e, const std::string &taskId) = 0;
};
} // namespace SDK_CLIENT_NAMESPACE
#endif // ARMONIK_SDK_ISERVICEINVOCATIONHANDLER_H
