#include "ChannelPool.h"

#include <armonik/client/channel/ChannelFactory.h>
#include <armonik/common/options/ControlPlane.h>
#include <armonik/common/utils/ChannelArguments.h>
#include <armonik/sdk/common/Version.h>
#include <grpcpp/create_channel.h>
#include <utility>

namespace ArmoniK {
namespace Sdk {
namespace Client {
namespace Internal {

std::shared_ptr<grpc::Channel> ChannelPool::AcquireChannel() {
  // A channel is thread-safe and multiplexes concurrent calls: hand them out in turn, without checkout
  return channels_[next_.fetch_add(1, std::memory_order_relaxed) % channels_.size()];
}

void ChannelPool::ReleaseChannel(std::shared_ptr<grpc::Channel>) {
  // The channels stay shared: nothing to return. gRPC reconnects a failed channel by itself.
}

ChannelPool::ChannelPool(ArmoniK::Sdk::Common::Properties properties, armonik::api::common::logger::Logger &logger)
    : properties_(std::move(properties)),
      channel_factory_(static_cast<armonik::api::common::utils::Configuration>(properties_.configuration), logger),
      logger_(logger.local({{"sdk_version", ArmoniK::Sdk::Common::getVersion()}})) {
  // Creating a channel opens no connection: each one connects on its first call
  const auto count = static_cast<std::size_t>(properties_.configuration.get_control_plane().getChannelCount());
  channels_.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    channels_.push_back(channel_factory_.create_channel());
  }
  logger_.debug("Created " + std::to_string(count) + " shared channels");
}
ChannelPool::~ChannelPool() = default;

ChannelPool::ChannelGuard::ChannelGuard(Internal::ChannelPool *pool) : pool_(pool) {
  if (pool_ != nullptr) {
    channel = pool_->AcquireChannel();
  }
}

ChannelPool::ChannelGuard::~ChannelGuard() {
  if (pool_ != nullptr) {
    pool_->ReleaseChannel(channel);
  }
}

ChannelPool::ChannelGuard ChannelPool::GetChannel() { return ChannelGuard(this); }

} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
