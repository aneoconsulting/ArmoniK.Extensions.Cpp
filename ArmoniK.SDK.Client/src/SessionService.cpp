#include "SessionService.h"
#include "SessionServiceImpl.h"
#include <string>

namespace SDK_CLIENT_NAMESPACE {

SessionService::SessionService(const ArmoniK::Sdk::Common::Properties &properties)
    : impl(new Internal::SessionServiceImpl(properties)) {}

const std::string &SessionService::getSession() const {
  ensure_valid();
  return impl->getSession();
}

std::vector<std::string>
ArmoniK::Sdk::Client::SessionService::Submit(const std::vector<Common::TaskPayload> &requests,
                                             const std::shared_ptr<IServiceInvocationHandler> &handler,
                                             const ArmoniK::Sdk::Common::TaskOptions &task_options) {
  ensure_valid();
  return impl->Submit(requests, handler, task_options);
}
std::vector<std::string>

SessionService::Submit(const std::vector<Common::TaskPayload> &requests,
                       const std::shared_ptr<IServiceInvocationHandler> &handler) {
  ensure_valid();
  return impl->Submit(requests, handler);
}
void SessionService::WaitResults(std::set<std::string> task_ids, WaitBehavior waitBehavior,
                                 const WaitOptions &options) {
  ensure_valid();
  impl->WaitResults(std::move(task_ids), waitBehavior, options);
}
SessionService &SessionService::operator=(SessionService &&other) noexcept = default;
void SessionService::ensure_valid() const {
  if (!impl) {
    throw std::runtime_error("Use after move");
  }
}
SessionService::SessionService(SessionService &&) noexcept = default;
SessionService::~SessionService() = default;
} // namespace SDK_CLIENT_NAMESPACE
