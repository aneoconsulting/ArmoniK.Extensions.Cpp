#include "SessionService.h"
#include "SessionServiceImpl.h"
#include <string>

namespace SDK_CLIENT_NAMESPACE {
SessionService::SessionService(const ArmoniK::SDK::Common::Properties &properties)
    : impl(new Internal::SessionServiceImpl(properties)) {}

std::string_view SessionService::getSession() const { return impl->getSession(); }

std::vector<std::string>
ArmoniK::SDK::Client::SessionService::Submit(const std::vector<Common::TaskRequest> &requests,
                                             const std::shared_ptr<IServiceInvocationHandler> &handler,
                                             const ArmoniK::SDK::Common::TaskOptions &task_options) {
  return impl->Submit(requests, handler, task_options);
}
std::vector<std::string>

SessionService::Submit(const std::vector<Common::TaskRequest> &requests,
                       const std::shared_ptr<IServiceInvocationHandler> &handler) {
  return impl->Submit(requests, handler);
}
} // namespace SDK_CLIENT_NAMESPACE
