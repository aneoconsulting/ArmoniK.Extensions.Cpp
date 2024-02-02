#include "ChannelPool.h"

#include <armonik/common/options/ControlPlane.h>
#include <armonik/common/utils/ChannelArguments.h>
#include <grpcpp/create_channel.h>
#include <fstream>
#include <utility>

namespace ArmoniK {
namespace Sdk {
namespace Client {
namespace Internal {

std::shared_ptr<grpc::Channel> ChannelPool::AcquireChannel() {
  std::shared_ptr<grpc::Channel> channel;
  {
    std::lock_guard<std::mutex> _(channel_mutex_);
    if (!channel_pool_.empty()) {
      channel = channel_pool_.front();
      channel_pool_.pop();
    }
  }

  if (channel != nullptr) {
    if (ShutdownOnFailure(channel)) {
      logger_.log(armonik::api::common::logger::Level::Debug, "Shutdown unhealthy channel");
    } else {
      logger_.log(armonik::api::common::logger::Level::Debug, "Acquired already existing channel from pool");
      return channel;
    }
  }
  std::string endpoint(properties_.configuration.get_control_plane().getEndpoint());
  auto scheme_delim = endpoint.find("://");
  if (scheme_delim != std::string::npos) {
    endpoint = endpoint.substr(scheme_delim + 3);
  }
  // TODO Handle TLS
  const auto &control_plane = properties_.configuration.get_control_plane();
  const auto get_key = [](const absl::string_view &path) -> std::string {
    std::ifstream file(path.data(), std::ios::in | std::ios::binary);
    std::ostringstream sstr;
    sstr << file.rdbuf();
    return sstr.str();
  };
  auto ca_cert_pem = get_key(control_plane.getCaCertPemPath());
  auto user_key_pem = get_key(control_plane.getUserKeyPemPath());
  auto user_cert_pem = get_key(control_plane.getUserCertPemPath());
  channel = grpc::CreateCustomChannel(
      endpoint,
      control_plane.isSslValidation() ? grpc::SslCredentials(grpc::SslCredentialsOptions{
                                            std::move(ca_cert_pem), std::move(user_key_pem), std::move(user_cert_pem)})
                                      : grpc::InsecureChannelCredentials(),
      armonik::api::common::utils::getChannelArguments(
          static_cast<armonik::api::common::utils::Configuration>(properties_.configuration)));
  logger_.log(armonik::api::common::logger::Level::Debug, "Created and acquired new channel from pool");
  return channel;
}

void ChannelPool::ReleaseChannel(std::shared_ptr<grpc::Channel> channel) {
  if (ShutdownOnFailure(channel)) {
    logger_.log(armonik::api::common::logger::Level::Debug, "Shutdown unhealthy channel");
  } else {
    logger_.log(armonik::api::common::logger::Level::Debug, "Released channel to pool");
    std::lock_guard<std::mutex> _(channel_mutex_);
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
  return false;
}

ChannelPool::ChannelPool(ArmoniK::Sdk::Common::Properties properties, armonik::api::common::logger::Logger &logger)
    : properties_(std::move(properties)), logger_(logger.local()) {}
ChannelPool::~ChannelPool() = default;

ChannelPool::ChannelGuard::ChannelGuard(Internal::ChannelPool *pool) : pool_(pool) {
  if (pool_ != nullptr) {
    channel = pool_->AcquireChannel();
  }
}

ChannelPool::ChannelGuard::~ChannelGuard() { pool_->ReleaseChannel(channel); }

ChannelPool::ChannelGuard ChannelPool::GetChannel() { return ChannelGuard(this); }

} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
