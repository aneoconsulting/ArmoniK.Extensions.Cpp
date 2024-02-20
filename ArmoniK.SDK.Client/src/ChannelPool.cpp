#include "ChannelPool.h"

#include <absl/types/optional.h>
#include <armonik/common/options/ControlPlane.h>
#include <armonik/common/utils/ChannelArguments.h>
#include <fstream>
#include <grpcpp/create_channel.h>
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

  // TODO Handle TLS
  channel = grpc::CreateCustomChannel(
      endpoint, credentials_,
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

absl::optional<std::string> get_key(const absl::string_view &path) {
  std::ifstream file(path.data(), std::ios::in | std::ios::binary);
  if (file.is_open()) {
    std::ostringstream sstr;
    sstr << file.rdbuf();
    return sstr.str();
  } else {
    return absl::nullopt;
  }
}

bool initialize_protocol_endpoint(const Common::Properties &properties_, std::string &endpoint) {
  absl::string_view endpoint_view = properties_.configuration.get_control_plane().getEndpoint();
  std::string protocol;
  protocol.reserve(5);
  const auto delim = endpoint_view.find("://");
  if (delim != absl::string_view::npos) {
    const auto tmp = endpoint_view.substr(delim);
    endpoint = {tmp.cbegin(), tmp.cend()};
    endpoint_view = endpoint_view.substr(0, delim);

    std::transform(endpoint_view.cbegin(), endpoint_view.cend(), std::back_inserter(protocol),
                   [](const char c) -> char { return static_cast<char>(tolower(c)); });
  }
  return protocol.back() == 's';
}

ChannelPool::ChannelPool(ArmoniK::Sdk::Common::Properties properties, armonik::api::common::logger::Logger &logger)
    : properties_(std::move(properties)), logger_(logger.local()) {
  const auto &control_plane = properties_.configuration.get_control_plane();
  const bool is_https = initialize_protocol_endpoint(properties_, endpoint);

  auto root_cert_pem = get_key(control_plane.getCaCertPemPath());
  auto user_private_pem = get_key(control_plane.getUserKeyPemPath());
  auto user_public_pem = get_key(control_plane.getUserCertPemPath());
  if (user_public_pem && user_private_pem && is_https) {
    if (!root_cert_pem) {
      logger_.log(armonik::api::common::logger::Level::Info, "No path for root certificate provided.");
    }
    credentials_ = grpc::SslCredentials(grpc::SslCredentialsOptions{
        root_cert_pem ? std::move(*root_cert_pem) : "", std::move(*user_private_pem), std::move(*user_public_pem)});
    is_secure = true;
  } else {
    credentials_ = grpc::InsecureChannelCredentials();
  }
}
ChannelPool::~ChannelPool() = default;

ChannelPool::ChannelGuard::ChannelGuard(Internal::ChannelPool *pool) : pool_(pool) {
  if (pool_ != nullptr) {
    channel = pool_->AcquireChannel();
  }
}

ChannelPool::ChannelGuard::~ChannelGuard() { pool_->ReleaseChannel(channel); }

ChannelPool::ChannelGuard ChannelPool::GetChannel() { return ChannelGuard(this); }

bool ChannelPool::isSecureChannel() const noexcept { return is_secure; }

} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
