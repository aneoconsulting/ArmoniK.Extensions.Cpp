#include "ChannelPool.h"

#include <armonik/common/options/ControlPlane.h>
#include <grpcpp/create_channel.h>
#include <shared_mutex>
#include <utility>

namespace SDK_CLIENT_NAMESPACE::Internal {

std::shared_ptr<grpc::Channel> ChannelPool::AcquireChannel() {
  std::shared_ptr<grpc::Channel> channel;
  std::cout << "Pool size " << channel_pool_.size() << std::endl;
  if (channel_pool_.size() != 0) {
    channel = channel_pool_.front();
    channel_pool_.pop();
  }

  if (channel != nullptr) {
    if (ShutdownOnFailure(channel)) {
      std::cout << "Shutdown unhealthy channel" << std::endl;
    } else {
      std::cout << "Acquired already existing channel from pool" << std::endl;
      return channel;
    }
  }

  std::shared_lock _(channel_mutex_);
  std::string endpoint(properties_.configuration.get_control_plane().getEndpoint());
  auto scheme_delim = endpoint.find("://");
  if (scheme_delim != std::string::npos) {
    endpoint = endpoint.substr(scheme_delim + 3);
  }
  channel = grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials());
  std::cout << "Created and acquired new channel from pool" << std::endl;
  return channel;
}

void ChannelPool::ReleaseChannel(std::shared_ptr<grpc::Channel> channel) {
  if (ShutdownOnFailure(channel)) {
    std::cout << "Shutdown unhealthy channel" << std::endl;
  } else {
    std::cout << "Released channel to pool" << std::endl;
    std::shared_lock _(channel_mutex_);
    channel_pool_.push(channel);
  }
}

bool ChannelPool::ShutdownOnFailure(std::shared_ptr<grpc::Channel> channel) {
  switch ((*channel).GetState(true)) {
  case GRPC_CHANNEL_CONNECTING:
    break;
  case GRPC_CHANNEL_IDLE:
    break;

  case GRPC_CHANNEL_SHUTDOWN:
    return true;
    break;

  case GRPC_CHANNEL_TRANSIENT_FAILURE:
    channel.reset();
    return true;
    break;

  case GRPC_CHANNEL_READY:
    break;

  default:
    return false;
    break;
  }
}

std::shared_ptr<grpc::Channel> ChannelPool::WithChannel() {
  auto guard = GetChannel();
  return guard.channel;
}

ChannelPool::ChannelPool(ArmoniK::Sdk::Common::Properties properties) : properties_(std::move(properties)) {}
ChannelPool::~ChannelPool() = default;

ChannelPool::ChannelGuard::ChannelGuard(const ArmoniK::Sdk::Common::Properties &properties)
    : pool_(new Internal::ChannelPool(properties)) {
  channel = (*pool_).AcquireChannel();
}

void ChannelPool::ChannelGuard::Dispose() { (*pool_).ReleaseChannel(channel); }

ChannelPool::ChannelGuard ChannelPool::GetChannel() { return ChannelGuard(properties_); }
} // namespace SDK_CLIENT_NAMESPACE::Internal
