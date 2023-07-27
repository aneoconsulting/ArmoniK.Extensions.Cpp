#include "IConfiguration.h"
#include <armonik/common/options/ComputePlane.h>
#include <armonik/common/options/ControlPlane.h>
#include <armonik/common/utils/IConfiguration.h>

#include <utility>

/**
 * @brief Proxy function definitions
 */

namespace SDK_COMMON_NAMESPACE {

ComputePlane::ComputePlane(const IConfiguration &configuration)
    : impl(std::make_shared<ArmoniK::Api::Common::options::ComputePlane>(*configuration.impl)) {}
std::string_view ComputePlane::get_server_address() const { return impl->get_server_address(); }
void ComputePlane::set_worker_address(std::string socket_address) {
  impl->set_worker_address(std::move(socket_address));
}
void ComputePlane::set_agent_address(std::string agent_address) { impl->set_agent_address(std::move(agent_address)); }
std::string_view ComputePlane::get_agent_address() const { return impl->get_agent_address(); }

ControlPlane::ControlPlane(const IConfiguration &config)
    : impl(std::make_shared<ArmoniK::Api::Common::options::ControlPlane>(*config.impl)) {}
std::string_view ControlPlane::getEndpoint() const { return impl->getEndpoint(); }
std::string_view ControlPlane::getUserCertPemPath() const { return impl->getUserCertPemPath(); }
std::string_view ControlPlane::getUserKeyPemPath() const { return impl->getUserKeyPemPath(); }
std::string_view ControlPlane::getUserP12Path() const { return impl->getUserP12Path(); }
std::string_view ControlPlane::getCaCertPemPath() const { return impl->getCaCertPemPath(); }
bool ControlPlane::isSslValidation() const { return impl->isSslValidation(); }

IConfiguration::IConfiguration() : impl(std::make_shared<ArmoniK::Api::Common::utils::IConfiguration>()) {}
IConfiguration::~IConfiguration() { impl.reset(); }
std::string IConfiguration::get(const std::string &string) const { return impl->get(string); }
void IConfiguration::set(const std::string &string, const std::string &value) { impl->set(string, value); }
void IConfiguration::set(const IConfiguration &other) { impl->set(*other.impl); }
const std::map<std::string, std::string> &IConfiguration::list() const { return impl->list(); }
IConfiguration &IConfiguration::add_json_configuration(std::string_view file_path) {
  impl->add_json_configuration(file_path);
  return *this;
}
IConfiguration &IConfiguration::add_env_configuration() {
  impl->add_env_configuration();
  return *this;
}
ComputePlane IConfiguration::get_compute_plane() const { return *this; }
ControlPlane IConfiguration::get_control_plane() const { return *this; }
IConfiguration::operator ArmoniK::Api::Common::utils::IConfiguration() { return *impl; }

} // namespace SDK_COMMON_NAMESPACE