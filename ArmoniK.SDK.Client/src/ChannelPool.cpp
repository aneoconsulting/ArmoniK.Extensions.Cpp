
#include "ChannelPool.h"

#include <armonik/common/options/ControlPlane.h>
#include <grpcpp/create_channel.h>
#include <utility>

std::shared_ptr<grpc::Channel> ChannelPool::GetChannel() {
  // TODO Setup TLS / mTLS support
  // TODO Setup channel recycling
  std::string endpoint(properties_.configuration.get_control_plane().getEndpoint());
  auto scheme_delim = endpoint.find("://");
  if (scheme_delim != std::string::npos) {
    endpoint = endpoint.substr(scheme_delim + 3);
  }
  return grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials());
}

ChannelPool::ChannelPool(ArmoniK::Sdk::Common::Properties properties) : properties_(std::move(properties)) {}
