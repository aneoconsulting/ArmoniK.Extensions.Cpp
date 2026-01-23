#include "armonik/sdk/common/Configuration.h"
#include "armonik/sdk/common/Utils.h"
#include <armonik/common/logger/level.h>
#include <armonik/common/options/ComputePlane.h>
#include <armonik/common/options/ControlPlane.h>
#include <armonik/common/utils/Configuration.h>

#include <utility>

/**
 * @brief Proxy function definitions
 */

namespace ArmoniK {
namespace Sdk {
namespace Common {

// Compute Plane
ComputePlane::ComputePlane(const Configuration &configuration)
    : impl(std::make_unique<armonik::api::common::options::ComputePlane>(*configuration.impl)) {}
ComputePlane::ComputePlane(const ComputePlane &computeplane)
    : impl(std::make_unique<armonik::api::common::options::ComputePlane>(*computeplane.impl)) {}
ComputePlane::ComputePlane(ComputePlane &&) noexcept = default;

ComputePlane &ComputePlane::operator=(const ComputePlane &computeplane) {
  impl = std::make_unique<armonik::api::common::options::ComputePlane>(*computeplane.impl);
  return *this;
}
ComputePlane &ComputePlane::operator=(ComputePlane &&) noexcept = default;
ComputePlane::~ComputePlane() = default;

absl::string_view ComputePlane::get_server_address() const { return impl->get_server_address(); }
void ComputePlane::set_worker_address(std::string socket_address) {
  impl->set_worker_address(std::move(socket_address));
}

void ComputePlane::set_agent_address(std::string agent_address) { impl->set_agent_address(std::move(agent_address)); }
absl::string_view ComputePlane::get_agent_address() const { return impl->get_agent_address(); }

const armonik::api::common::options::ComputePlane &ComputePlane::get_impl() const {
  const static armonik::api::common::options::ComputePlane default_config =
      armonik::api::common::options::ComputePlane(armonik::api::common::utils::Configuration());
  if (impl) {
    return *impl;
  }
  return default_config;
}
armonik::api::common::options::ComputePlane &ComputePlane::set_impl() {
  if (!impl) {
    impl = std::make_unique<armonik::api::common::options::ComputePlane>(armonik::api::common::utils::Configuration());
  }
  return *impl;
}

// Control Plane

namespace {
int getBatchSize(const Configuration &config, const std::string &key) {
  auto batch_size_str = config.get(key);
  int batch_size = 1; // Default value
  try {
    batch_size = std::stoi(batch_size_str);
  } catch (...) {
    // Ignore invalid value and keep default
  }
  return batch_size > 0 ? batch_size : 1; // Ensure positive value
}
} // namespace

ControlPlane::ControlPlane(const Configuration &config)
    : impl(std::make_unique<armonik::api::common::options::ControlPlane>(*config.impl)),
      wait_batch_size_(getBatchSize(config, "GrpcClient__WaitBatchSize")),
      submit_batch_size_(getBatchSize(config, "GrpcClient__SubmitBatchSize")) {}

ControlPlane::ControlPlane(const ControlPlane &controlplane)
    : impl(std::make_unique<armonik::api::common::options::ControlPlane>(*controlplane.impl)),
      wait_batch_size_(controlplane.wait_batch_size_), submit_batch_size_(controlplane.submit_batch_size_) {}
ControlPlane::ControlPlane(ControlPlane &&) noexcept = default;

ControlPlane &ControlPlane::operator=(const ControlPlane &controlplane) {
  impl = std::make_unique<armonik::api::common::options::ControlPlane>(*controlplane.impl);
  wait_batch_size_ = controlplane.wait_batch_size_;
  submit_batch_size_ = controlplane.submit_batch_size_;
  return *this;
}
ControlPlane &ControlPlane::operator=(ControlPlane &&) noexcept = default;
ControlPlane::~ControlPlane() = default;

absl::string_view ControlPlane::getEndpoint() const { return impl->getEndpoint(); }
absl::string_view ControlPlane::getUserCertPemPath() const { return impl->getUserCertPemPath(); }
absl::string_view ControlPlane::getUserKeyPemPath() const { return impl->getUserKeyPemPath(); }
absl::string_view ControlPlane::getUserP12Path() const { return impl->getUserP12Path(); }
absl::string_view ControlPlane::getCaCertPemPath() const { return impl->getCaCertPemPath(); }
bool ControlPlane::isSslValidation() const { return impl->isSslValidation(); }

int ControlPlane::getWaitBatchSize() const { return wait_batch_size_; }
int ControlPlane::getSubmitBatchSize() const { return submit_batch_size_; }

const armonik::api::common::options::ControlPlane &ControlPlane::get_impl() const {
  const static armonik::api::common::options::ControlPlane default_config =
      armonik::api::common::options::ControlPlane(armonik::api::common::utils::Configuration());
  if (impl) {
    return *impl;
  }
  return default_config;
}

armonik::api::common::options::ControlPlane &ControlPlane::set_impl() {
  if (!impl) {
    impl = std::make_unique<armonik::api::common::options::ControlPlane>(armonik::api::common::utils::Configuration());
  }
  return *impl;
}

// Configuration

Configuration::Configuration() : impl(std::make_unique<armonik::api::common::utils::Configuration>()) {}
Configuration::Configuration(const Configuration &config)
    : impl(std::make_unique<armonik::api::common::utils::Configuration>(*config.impl)) {}
Configuration::Configuration(Configuration &&) noexcept = default;

Configuration::~Configuration() = default;

Configuration &Configuration::operator=(Configuration &&) noexcept = default;
Configuration &Configuration::operator=(const Configuration &config) {
  impl = std::make_unique<armonik::api::common::utils::Configuration>(*config.impl);
  return *this;
}

std::string Configuration::get(const std::string &key) const { return get_impl().get(key); }
void Configuration::set(const std::string &key, const std::string &value) { set_impl().set(key, value); }

const std::map<std::string, std::string> &Configuration::list() const { return get_impl().list(); }
Configuration &Configuration::add_json_configuration(absl::string_view file_path) {
  set_impl().add_json_configuration(file_path);
  return *this;
}

Configuration &Configuration::add_env_configuration() {
  set_impl().add_env_configuration();
  return *this;
}

ComputePlane Configuration::get_compute_plane() const { return *this; }
ControlPlane Configuration::get_control_plane() const { return *this; }

Configuration::operator armonik::api::common::utils::Configuration() { return get_impl(); }

armonik::api::common::utils::Configuration &Configuration::set_impl() {
  if (!impl) {
    impl = std::make_unique<armonik::api::common::utils::Configuration>();
  }
  return *impl;
}

const armonik::api::common::utils::Configuration &Configuration::get_impl() const {
  const static armonik::api::common::utils::Configuration default_config;
  if (impl) {
    return *impl;
  }
  return default_config;
}

armonik::api::common::logger::Level Configuration::get_log_level() const {
  auto rawLevel = ArmoniK::Sdk::Common::to_capitalized(get("Serilog__MinimumLevel"));

  if (level_name(armonik::api::common::logger::Level::Verbose) == rawLevel) {
    return armonik::api::common::logger::Level::Verbose;
  } else if (level_name(armonik::api::common::logger::Level::Debug) == rawLevel) {
    return armonik::api::common::logger::Level::Debug;
  } else if (level_name(armonik::api::common::logger::Level::Info) == rawLevel) {
    return armonik::api::common::logger::Level::Info;
  } else if (level_name(armonik::api::common::logger::Level::Warning) == rawLevel) {
    return armonik::api::common::logger::Level::Warning;
  } else if (level_name(armonik::api::common::logger::Level::Error) == rawLevel) {
    return armonik::api::common::logger::Level::Error;
  } else if (level_name(armonik::api::common::logger::Level::Fatal) == rawLevel) {
    return armonik::api::common::logger::Level::Fatal;
  }
  return armonik::api::common::logger::Level::Info;
}

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
