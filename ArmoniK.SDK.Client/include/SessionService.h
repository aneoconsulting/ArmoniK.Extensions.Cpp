#ifndef ARMONIK_EXTENSIONS_CPP_SESSIONSERVICE_H
#define ARMONIK_EXTENSIONS_CPP_SESSIONSERVICE_H

#include <memory>
#include <string_view>
#include <vector>

namespace ArmoniK::SDK::Common {
class TaskOptions;
class Properties;
class TaskRequest;
} // namespace ArmoniK::SDK::Common

namespace SDK_CLIENT_NAMESPACE::Internal {
class SessionServiceImpl;
}

namespace SDK_CLIENT_NAMESPACE {
class IServiceInvocationHandler;

class SessionService {
private:
  std::shared_ptr<SDK_CLIENT_NAMESPACE::Internal::SessionServiceImpl> impl;

public:
  SessionService() = delete;
  explicit SessionService(const ArmoniK::SDK::Common::Properties &properties);

  std::vector<std::string> Submit(const std::vector<Common::TaskRequest> &requests,
                                  std::shared_ptr<IServiceInvocationHandler> handler,
                                  const ArmoniK::SDK::Common::TaskOptions &task_options);

  [[nodiscard]] std::string_view getSession() const;
};
} // namespace SDK_CLIENT_NAMESPACE
#endif // ARMONIK_EXTENSIONS_CPP_SESSIONSERVICE_H
