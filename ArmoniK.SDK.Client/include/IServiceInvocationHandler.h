#ifndef ARMONIK_SDK_ISERVICEINVOCATIONHANDLER_H
#define ARMONIK_SDK_ISERVICEINVOCATIONHANDLER_H

#include <string>

namespace SDK_CLIENT_NAMESPACE {

class IServiceInvocationHandler {
public:
  virtual void HandleResponse(const std::string &result_payload, const std::string &taskId) = 0;
  virtual void HandleError(const std::exception &e, const std::string &taskId) = 0;
};
} // namespace SDK_CLIENT_NAMESPACE
#endif // ARMONIK_SDK_ISERVICEINVOCATIONHANDLER_H
