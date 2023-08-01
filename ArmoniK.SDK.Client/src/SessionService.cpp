#include "SessionService.h"
#include "SessionServiceImpl.h"
#include <string>

namespace SDK_CLIENT_NAMESPACE {
SessionService::SessionService(const ArmoniK::Sdk::Common::Properties &properties)
    : impl(new Internal::SessionServiceImpl(properties)) {}

std::string_view SessionService::getSession() const { return impl->getSession(); }

std::vector<std::string>
ArmoniK::Sdk::Client::SessionService::Submit(const std::vector<Common::TaskPayload> &requests,
                                             std::shared_ptr<IServiceInvocationHandler> handler,
                                             const ArmoniK::Sdk::Common::TaskOptions &task_options) {
  return impl->Submit(requests, std::move(handler), task_options);
}
std::vector<std::string>

SessionService::Submit(const std::vector<Common::TaskPayload> &requests,
                       std::shared_ptr<IServiceInvocationHandler> handler) {
  return impl->Submit(requests, std::move(handler));
}
void SessionService::WaitResults(std::set<std::string> task_ids, WaitBehavior waitBehavior,
                                 const WaitOptions &options) {
  impl->WaitResults(std::move(task_ids), waitBehavior, options);
}
} // namespace SDK_CLIENT_NAMESPACE
