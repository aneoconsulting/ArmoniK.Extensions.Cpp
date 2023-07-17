
#include "ChannelPool.h"

#include <armonik/common/options/ControlPlane.h>
#include <grpcpp/create_channel.h>
#include <utility>

std::shared_ptr<grpc::Channel> ChannelPool::GetChannel() {
  // TODO Setup TLS / mTLS support
  return grpc::CreateChannel(std::string(properties_.configuration.get_control_plane().getEndpoint()),
                             grpc::InsecureChannelCredentials());
}

ChannelPool::ChannelPool(ArmoniK::SDK::Common::Properties properties) : properties_(std::move(properties)) {}
