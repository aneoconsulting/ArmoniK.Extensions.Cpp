#include "armonik/sdk/client/SessionService.h"
#include "SessionServiceImpl.h"
#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Client {

SessionService::SessionService(const ArmoniK::Sdk::Common::Properties &properties,
                               armonik::api::common::logger::Logger &logger)
    : impl(new Internal::SessionServiceImpl(properties, logger)), logger_(logger.local()) {}

const std::string &SessionService::getSession() const {
  ensure_valid();
  return impl->getSession();
}

std::vector<std::string>
ArmoniK::Sdk::Client::SessionService::Submit(const std::vector<Common::TaskPayload> &requests,
                                             std::shared_ptr<IServiceInvocationHandler> handler,
                                             const ArmoniK::Sdk::Common::TaskOptions &task_options) {
  ensure_valid();
  return impl->Submit(requests, std::move(handler), task_options);
}
std::vector<std::string>

SessionService::Submit(const std::vector<Common::TaskPayload> &requests,
                       std::shared_ptr<IServiceInvocationHandler> handler) {
  ensure_valid();
  return impl->Submit(requests, std::move(handler));
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
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
