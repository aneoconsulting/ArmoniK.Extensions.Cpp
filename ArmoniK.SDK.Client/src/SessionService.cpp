#include <string>
#include "SessionService.h"
#include "SessionServiceImpl.h"

ArmoniK::SDK::Client::SessionService::SessionService(const ArmoniK::SDK::Common::Properties &properties) : impl(new Internal::SessionServiceImpl(properties)) {}

std::string_view ArmoniK::SDK::Client::SessionService::getSession() const {
    return impl->getSession();
}

std::vector<std::string> ArmoniK::SDK::Client::SessionService::Submit(const std::vector<TaskRequest> &requests,
                                                                      std::shared_ptr<IServiceInvocationHandler> handler,
                                                                      const ArmoniK::SDK::Common::TaskOptions &task_options) {
    return impl->Submit(requests, std::move(handler), task_options);
}
