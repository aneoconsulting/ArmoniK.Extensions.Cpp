#include "ChannelPool.h"

#include <armonik/client/channel/ChannelFactory.h>
#include <grpcpp/create_channel.h>
#include <utility>

namespace ArmoniK {
namespace Sdk {
namespace Client {
namespace Internal {

std::shared_ptr<grpc::Channel> ChannelPool::AcquireChannel() {
  std::shared_ptr<grpc::Channel> channel;
  {
    std::lock_guard<std::mutex> lock(channel_mutex_);
    if (!channel_pool_.empty()) {
      channel = std::move(channel_pool_.front());
      channel_pool_.pop();
    }
  }

  if (channel) {
    if (ShutdownOnFailure(channel)) {
      logger_.debug("Shutdown unhealthy channel");
      channel.reset();
    } else {
      logger_.debug("Acquired existing channel from pool");
      return channel;
    }
  }

  channel = factory_.create_channel();
  logger_.debug("Created new channel");
  return channel;
}

void ChannelPool::ReleaseChannel(std::shared_ptr<grpc::Channel> channel) {
  if (ShutdownOnFailure(channel)) {
    logger_.debug("Shutdown unhealthy channel");
  } else {
    logger_.debug("Released channel to pool");
    std::lock_guard<std::mutex> lock(channel_mutex_);
    channel_pool_.push(std::move(channel));
  }
}

bool ChannelPool::ShutdownOnFailure(std::shared_ptr<grpc::Channel> channel) {
  switch (channel->GetState(true)) {
  case GRPC_CHANNEL_SHUTDOWN:
  case GRPC_CHANNEL_TRANSIENT_FAILURE:
    return true;
  default:
    return false;
  }
}

  ChannelPool::ChannelPool(ArmoniK::Sdk::Common::Properties properties, armonik::api::common::logger::Logger &logger)
    : properties_(std::move(properties)),
      factory_(static_cast<armonik::api::common::utils::Configuration>(properties_.configuration), logger),
      logger_(logger.local())
{}

ChannelPool::~ChannelPool() = default;

ChannelPool::ChannelGuard::ChannelGuard(ChannelPool *pool) : pool_(pool) {
  if (pool_) {
    channel = pool_->AcquireChannel();
  }
}

ChannelPool::ChannelGuard::~ChannelGuard() {
  if (pool_ && channel) {
    pool_->ReleaseChannel(channel);
  }
}

ChannelPool::ChannelGuard ChannelPool::GetChannel() { return ChannelGuard(this); }

} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK