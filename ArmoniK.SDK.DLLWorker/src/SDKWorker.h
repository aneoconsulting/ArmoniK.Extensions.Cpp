#ifndef ARMONIK_SDK_SDKWORKER_H
#define ARMONIK_SDK_SDKWORKER_H

#include <ApplicationManager.h>
#include <worker_service.grpc.pb.h>

namespace SDK_DLLWORKER_NAMESPACE {
class SDKWorker : public armonik::api::grpc::v1::worker::Worker::Service {
public:
  grpc::Status Process(::grpc::ServerContext *context,
                       ::grpc::ServerReader<::armonik::api::grpc::v1::worker::ProcessRequest> *reader,
                       ::armonik::api::grpc::v1::worker::ProcessReply *response) override;
  grpc::Status HealthCheck(::grpc::ServerContext *context, const ::armonik::api::grpc::v1::Empty *request,
                           ::armonik::api::grpc::v1::worker::HealthCheckReply *response) override;

private:
  ArmoniK::SDK::Worker::ApplicationManager manager;
};
} // namespace SDK_DLLWORKER_NAMESPACE

#endif // ARMONIK_SDK_SDKWORKER_H
