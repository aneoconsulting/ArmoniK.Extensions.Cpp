#include "Configuration.h"
#include <armonik/common/options/ComputePlane.h>
#include <armonik/common/options/ControlPlane.h>
#include <armonik/common/utils/Configuration.h>

#include <utility>

/**
 * @brief Proxy function definitions
 */

namespace SDK_COMMON_NAMESPACE {

ComputePlane::ComputePlane(const Configuration &configuration)
    : impl(std::make_unique<ArmoniK::Api::Common::options::ComputePlane>(*configuration.impl)) {}
std::string_view ComputePlane::get_server_address() const { return impl->get_server_address(); }
void ComputePlane::set_worker_address(std::string socket_address) {
  impl->set_worker_address(std::move(socket_address));
}
void ComputePlane::set_agent_address(std::string agent_address) { impl->set_agent_address(std::move(agent_address)); }
std::string_view ComputePlane::get_agent_address() const { return impl->get_agent_address(); }
ComputePlane::ComputePlane(const ComputePlane &computeplane)
    : impl(std::make_unique<ArmoniK::Api::Common::options::ComputePlane>(*computeplane.impl)) {}
ComputePlane::ComputePlane(ComputePlane &&) noexcept = default;
ComputePlane &ComputePlane::operator=(const ComputePlane &computeplane) {
  impl = std::make_unique<ArmoniK::Api::Common::options::ComputePlane>(*computeplane.impl);
  return *this;
}
ComputePlane &ComputePlane::operator=(ComputePlane &&) noexcept = default;
const ArmoniK::Api::Common::options::ComputePlane &ComputePlane::get_impl() const {
  const static ArmoniK::Api::Common::options::ComputePlane default_config =
      ArmoniK::Api::Common::options::ComputePlane(ArmoniK::Api::Common::utils::Configuration());
  if (impl) {
    return *impl;
  }
  return default_config;
}
ArmoniK::Api::Common::options::ComputePlane &ComputePlane::set_impl() {
  if (!impl) {
    impl = std::make_unique<ArmoniK::Api::Common::options::ComputePlane>(ArmoniK::Api::Common::utils::Configuration());
  }
  return *impl;
}

ControlPlane::ControlPlane(const Configuration &config)
    : impl(std::make_unique<ArmoniK::Api::Common::options::ControlPlane>(*config.impl)) {}
std::string_view ControlPlane::getEndpoint() const { return impl->getEndpoint(); }
std::string_view ControlPlane::getUserCertPemPath() const { return impl->getUserCertPemPath(); }
std::string_view ControlPlane::getUserKeyPemPath() const { return impl->getUserKeyPemPath(); }
std::string_view ControlPlane::getUserP12Path() const { return impl->getUserP12Path(); }
std::string_view ControlPlane::getCaCertPemPath() const { return impl->getCaCertPemPath(); }
bool ControlPlane::isSslValidation() const { return impl->isSslValidation(); }
ControlPlane::ControlPlane(const ControlPlane &controlplane)
    : impl(std::make_unique<ArmoniK::Api::Common::options::ControlPlane>(*controlplane.impl)) {}
ControlPlane::ControlPlane(ControlPlane &&) noexcept = default;
ControlPlane &ControlPlane::operator=(const ControlPlane &controlplane) {
  impl = std::make_unique<ArmoniK::Api::Common::options::ControlPlane>(*controlplane.impl);
  return *this;
}
ControlPlane &ControlPlane::operator=(ControlPlane &&) noexcept = default;
const ArmoniK::Api::Common::options::ControlPlane &ControlPlane::get_impl() const {
  const static ArmoniK::Api::Common::options::ControlPlane default_config =
      ArmoniK::Api::Common::options::ControlPlane(ArmoniK::Api::Common::utils::Configuration());
  if (impl) {
    return *impl;
  }
  return default_config;
}

ArmoniK::Api::Common::options::ControlPlane &ControlPlane::set_impl() {
  if (!impl) {
    impl = std::make_unique<ArmoniK::Api::Common::options::ControlPlane>(ArmoniK::Api::Common::utils::Configuration());
  }
  return *impl;
}

Configuration::Configuration() : impl(std::make_unique<ArmoniK::Api::Common::utils::Configuration>()) {}
Configuration::~Configuration() = default;
std::string Configuration::get(const std::string &key) const { return get_impl().get(key); }
void Configuration::set(const std::string &key, const std::string &value) { set_impl().set(key, value); }
const std::map<std::string, std::string> &Configuration::list() const { return get_impl().list(); }
Configuration &Configuration::add_json_configuration(std::string_view file_path) {
  set_impl().add_json_configuration(file_path);
  return *this;
}
Configuration &Configuration::add_env_configuration() {
  set_impl().add_env_configuration();
  return *this;
}
ComputePlane Configuration::get_compute_plane() const { return *this; }
ControlPlane Configuration::get_control_plane() const { return *this; }
Configuration::operator ArmoniK::Api::Common::utils::Configuration() { return get_impl(); }
Configuration::Configuration(const Configuration &config)
    : impl(std::make_unique<ArmoniK::Api::Common::utils::Configuration>(*config.impl)) {}
Configuration &Configuration::operator=(const Configuration &config) {
  impl = std::make_unique<ArmoniK::Api::Common::utils::Configuration>(*config.impl);
  return *this;
}
ArmoniK::Api::Common::utils::Configuration &Configuration::set_impl() {
  if (!impl) {
    impl = std::make_unique<ArmoniK::Api::Common::utils::Configuration>();
  }
  return *impl;
}
const ArmoniK::Api::Common::utils::Configuration &Configuration::get_impl() const {
  const static ArmoniK::Api::Common::utils::Configuration default_config;
  if (impl) {
    return *impl;
  }
  return default_config;
}
Configuration::Configuration(Configuration &&) noexcept = default;
Configuration &Configuration::operator=(Configuration &&) noexcept = default;

} // namespace SDK_COMMON_NAMESPACE
