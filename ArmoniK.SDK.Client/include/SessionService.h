#ifndef ARMONIK_EXTENSIONS_CPP_SESSIONSERVICE_H
#define ARMONIK_EXTENSIONS_CPP_SESSIONSERVICE_H

#include <grpc++/grpc++.h>
#include <armonik/client/SubmitterClient.h>
#include "TaskOptions.h"
#include "TaskRequest.h"
namespace SDK_CLIENT_NAMESPACE {

    class SessionService {
    private:
        std::string session;
        SubmitterClient client;

        std::string CreateSession();

    public:
        explicit SessionService(std::shared_ptr<grpc::Channel> channel);
        explicit SessionService(std::shared_ptr<grpc::Channel> channel, ArmoniK::SDK::Common::TaskOptions taskOptions);

        SessionService() = delete;

        std::vector<std::string> Submit();

    };
}
#endif //ARMONIK_EXTENSIONS_CPP_SESSIONSERVICE_H
