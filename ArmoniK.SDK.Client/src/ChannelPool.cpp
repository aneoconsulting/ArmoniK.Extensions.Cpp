#include "ChannelPool.h"

#include <armonik/common/options/ControlPlane.h>
#include <armonik/common/utils/ChannelArguments.h>
#include <fstream>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/security/tls_credentials_options.h>
#include <sstream>
#include <utility>

namespace ArmoniK {
namespace Sdk {
namespace Client {
namespace Internal {

using namespace armonik::api::common;
using namespace grpc::experimental;

const std::string root_self_signed = R"(-----BEGIN CERTIFICATE REQUEST-----
MIIEhjCCAm4CAQAwQTELMAkGA1UEBhMCRlIxEzARBgNVBAgMClNvbWUtU3RhdGUx
DjAMBgNVBAcMBVBhcmlzMQ0wCwYDVQQKDARBbmVvMIICIjANBgkqhkiG9w0BAQEF
AAOCAg8AMIICCgKCAgEA3lBl8so+JRen+tfbrytXMmYAvjt/WquctbkbFIN6prdp
uShiRb6kX9jobcOQCleQ08LBLPPoQ7AemymPxT0dq+YPFw33LgrIBpKe0JWYzujB
Ujj39b1EmKonnsx+C6DL2KSkIf7ayoBNdjDgunWkVC4M6hoJE7XYyZ78HKndfuvL
C4zs3o1EizvSpp+O/IzD/y5pnZEBoxMLCRNB8vD7w7mQMhx+6Amx7KkfCDKLOQO4
/K2x8r4Y65+IvxFMyxUsR1Z5XPVv37u7u2akbh3HlUE+m0xzVOk+BmHFYxm/eEAF
4p1Jt3bZWu03eF4f8tmgN31Rv0uV+BRN7na44inXNnyd+2qczaCI1IQmsy23Vu0A
eX61Gu06ifViJAybbcWll3VQjWqj5XtsN2+yr2bGfZw8fpjGXVWTL0+nZSqZPWSo
IYlXMHjcygWyMJXTMVTTN+fV7dd9s1LFVnpdHFFOtmRzY8FlRRSpOoqG8XQXXsk0
pE9904wHaXcwSEe4KtuzgZgNngRCtT61G6k+onhrGa6UVCKpfvMYtS3NEsMNNYsY
I5Hn7Unj/0xBO6IM5Os6PImWWMk8rLSXC3IdtEAHgShS+/xbh2ZVOveSeMXWaecm
u2RIe5wQa5ZXLr03XtkdMB1pebJbdoFrs0ev/sklk1dZfbX06vJSd8eokM9oIIcC
AwEAAaAAMA0GCSqGSIb3DQEBCwUAA4ICAQCr75dBYjypzqDqQ6TiKWuYO8rq6TIh
pdZHw5ystwvD6sn+tPbc7iNbnvDF6GeTgMdKAuwNz0YJMZq9v39hZzTCyMqRLNlT
TU3kYaTWGHDK0HE2O3pHKppHAc2YbAsSxuS8KMHx0wW0abVHiEeudc/nULJppX1/
ObouzLGSJJwZctXEzk/Ye7bD1sneSqVnrdFD1IOBVQVRGoJznAt7WWxvGk9LPW51
+MybzTilL4rk5+ezA4UCIMrQCDwZcI+UCcKqDajDz+7kn81f1K4g1G6dTh+M8qIV
lx6/Bfy3P6DHF1ww0i/hRQht1O9cyUo3mDZzAq20OsIDvkhjNGma/IEbkZ9z0P5C
/5YwAW+GuwG2GrD016y5OjZVrAG/KIfyS6FLQfgN/ww5Y9tK6vO5XkelED7zNPrq
em1zkId2H0Az5dIC2OpnAg3+NuGrehfIXziiY+8MGIivqI/Rulnv7m2l2vjHi66K
GztDm5ohMdfjitFIfPDFYPMH7KES4vivic8zlq9FJYNp8tUYEBR1wW7W03IJPm6e
pUwvXHPjId/qBjlBixZt2ZqC8X4S95wAfVjtS3O33Zsm4oevwlvywfYIK8nTG5SD
bDCNVTg3w/OQLQQdWUl6FunmYinukBgmqnsJnwgrhzBENbmgbgfOZZWGtG5ODENb
wc+KqiSg9c9iqA==
-----END CERTIFICATE REQUEST-----)";

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
      logger_.log(logger::Level::Debug, "Shutdown unhealthy channel");
    } else {
      logger_.log(logger::Level::Debug, "Acquired already existing channel from pool");
      return channel;
    }
  }

  channel = grpc::CreateCustomChannel(
      endpoint, credentials_, utils::getChannelArguments(static_cast<utils::Configuration>(properties_.configuration)));
  logger_.log(logger::Level::Debug, "Created and acquired new channel from pool");
  return channel;
}

void ChannelPool::ReleaseChannel(std::shared_ptr<grpc::Channel> channel) {
  if (ShutdownOnFailure(channel)) {
    logger_.log(logger::Level::Debug, "Shutdown unhealthy channel");
  } else {
    logger_.log(logger::Level::Debug, "Released channel to pool");
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

std::string get_key(const absl::string_view &path) {
  std::ifstream file(path.data(), std::ios::in | std::ios::binary);
  if (file.is_open()) {
    std::ostringstream sstr;
    sstr << file.rdbuf();
    return sstr.str();
  } else {
    return {};
  }
}

bool initialize_protocol_endpoint(const Common::ControlPlane &controlPlane, std::string &endpoint) {
  absl::string_view endpoint_view = controlPlane.getEndpoint();
  const auto delim = endpoint_view.find("://");
  if (delim != absl::string_view::npos) {
    const auto tmp = endpoint_view.substr(delim + 3);
    endpoint_view = endpoint_view.substr(0, delim);
    endpoint = {tmp.cbegin(), tmp.cend()};
  }
  return endpoint_view.back() == 's' || endpoint_view.back() == 'S';
}

std::shared_ptr<CertificateProviderInterface> create_certificate_provider(const std::string &rootCertificate,
                                                                          const std::string &userPublicPem,
                                                                          const std::string &userPrivatePem) {
  if (rootCertificate.empty()) {
    return std::make_shared<StaticDataCertificateProvider>(
        std::vector<IdentityKeyCertPair>{IdentityKeyCertPair{userPrivatePem, userPublicPem}});
  } else if (userPrivatePem.empty() || userPublicPem.empty()) {
    return std::make_shared<StaticDataCertificateProvider>(rootCertificate);
  } else {
    return std::make_shared<StaticDataCertificateProvider>(
        rootCertificate, std::vector<IdentityKeyCertPair>{IdentityKeyCertPair{userPrivatePem, userPublicPem}});
  }
}

ChannelPool::ChannelPool(Common::Properties properties, logger::Logger &logger)
    : properties_(std::move(properties)), logger_(logger.local()) {
  // WARNING: control_plane is created on the fly, it is NOT a reference.
  const auto control_plane = properties_.configuration.get_control_plane();
  const bool is_https = initialize_protocol_endpoint(control_plane, endpoint);

  auto root_cert_pem = get_key(control_plane.getCaCertPemPath());
  auto user_private_pem = get_key(control_plane.getUserKeyPemPath());
  auto user_public_pem = get_key(control_plane.getUserCertPemPath());
  if (is_https) {
    if (!user_private_pem.empty() && !user_public_pem.empty()) {
      if (control_plane.isSslValidation()) {
        credentials_ = grpc::SslCredentials(grpc::SslCredentialsOptions{
            std::move(root_cert_pem), std::move(user_private_pem), std::move(user_public_pem)});
      } else {
        TlsChannelCredentialsOptions tls_options;
        tls_options.set_verify_server_certs(false);
        tls_options.set_certificate_provider(
            create_certificate_provider(root_cert_pem, user_public_pem, user_private_pem));
        credentials_ = TlsCredentials(tls_options);
      }
    } else {
      const std::string &root_cert = !root_cert_pem.empty() ? root_cert_pem : root_self_signed;
      TlsChannelCredentialsOptions tls_options;
      tls_options.set_certificate_provider(create_certificate_provider(root_cert, user_public_pem, user_private_pem));
      tls_options.set_verify_server_certs(control_plane.isSslValidation());
      credentials_ = TlsCredentials(tls_options);
    }
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
