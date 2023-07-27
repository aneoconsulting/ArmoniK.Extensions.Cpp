#include "SDKWorker.h"
#include <armonik/sdk/worker/ApplicationManager.h>
grpc::Status SDK_DLLWORKER_NAMESPACE::SDKWorker::Process(
    ::grpc::ServerContext *context, ::grpc::ServerReader<::armonik::api::grpc::v1::worker::ProcessRequest> *reader,
    ::armonik::api::grpc::v1::worker::ProcessReply *response) {}
grpc::Status
SDK_DLLWORKER_NAMESPACE::SDKWorker::HealthCheck(::grpc::ServerContext *context,
                                                const ::armonik::api::grpc::v1::Empty *request,
                                                ::armonik::api::grpc::v1::worker::HealthCheckReply *response) {
  return Service::HealthCheck(context, request, response);
}
