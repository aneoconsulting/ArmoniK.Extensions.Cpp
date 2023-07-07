#include "SessionService.h"



SDK_CLIENT_NAMESPACE::SessionService::SessionService(std::shared_ptr<grpc::Channel> channel) : client(armonik::api::grpc::v1::submitter::Submitter::NewStub(channel)) {
    session = client.create_session()
}

std::string SDK_CLIENT_NAMESPACE::SessionService::CreateSession() {
    return std::string();
}

std::vector<std::string> SDK_CLIENT_NAMESPACE::SessionService::Submit() {
    return std::vector<std::string>();
}

SDK_CLIENT_NAMESPACE::SessionService::SessionService(std::shared_ptr<grpc::Channel> channel,
                                                     ArmoniK::SDK::Common::TaskOptions taskOptions) {

}
