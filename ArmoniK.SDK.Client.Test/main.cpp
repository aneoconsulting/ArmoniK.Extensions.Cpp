#include <iostream>
#include <grpc++/grpc++.h>
#include <armonik/client/SubmitterClient.h>
#include "submitter_service.grpc.pb.h"

int main() {
    std::cout << "Hello, World!" << std::endl;
    auto channel = grpc::CreateChannel("armonik.local:5001", grpc::InsecureChannelCredentials());
    SubmitterClient client(armonik::api::grpc::v1::submitter::Submitter::NewStub(channel));
    return 0;
}