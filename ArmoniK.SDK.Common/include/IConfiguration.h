#ifndef ARMONIK_SDK_ICONFIGURATION_H
#define ARMONIK_SDK_ICONFIGURATION_H

#include <map>
#include <memory>
#include <string>

/**
 * @brief This file declares proxy objects for the IConfiguration objects
 */

namespace ArmoniK::Api::Common::utils {
class IConfiguration;
}

namespace ArmoniK::Api::Common::options {
class ComputePlane;
class ControlPlane;
} // namespace ArmoniK::Api::Common::options

namespace SDK_COMMON_NAMESPACE {
class IConfiguration;

class ControlPlane {
public:
  /**
   * @brief Constructs a ControlPlane object with the given configuration.
   * @param configuration The IConfiguration object containing address information.
   */
  ControlPlane(const IConfiguration &config);

  /**
   * @brief ArmoniK control plane endpoint
   * @return Endpoint address
   */
  [[nodiscard]] std::string_view getEndpoint() const;

  /**
   * @brief Path to the client's certificate in PEM format
   * @return Client certificate's path
   */
  [[nodiscard]] std::string_view getUserCertPemPath() const;

  /**
   * @brief Path to the client's key in PEM format (PKCS#1 or PKCS#8)
   * @return Client key path
   */
  [[nodiscard]] std::string_view getUserKeyPemPath() const;

  /**
   * @brief Path to the client's PKCS#12 certificate/key
   * @return Client P12 path
   */
  [[nodiscard]] std::string_view getUserP12Path() const;

  /**
   * @brief Path to the server's CA certificate
   * @return CA certificate path
   */
  [[nodiscard]] std::string_view getCaCertPemPath() const;

  /**
   * @brief Is SSL validation enabled ?
   * @return True if SSL validation is enabled, false otherwise
   */
  [[nodiscard]] bool isSslValidation() const;

private:
  std::shared_ptr<ArmoniK::Api::Common::options::ControlPlane> impl;
};

class ComputePlane {
public:
  /**
   * @brief Constructs a ComputePlane object with the given configuration.
   * @param configuration The IConfiguration object containing address information.
   */
  ComputePlane(const IConfiguration &configuration);

  /**
   * @brief Returns the server address.
   * @return A reference to the server address string.
   */
  [[nodiscard]] std::string_view get_server_address() const;

  /**
   * @brief Sets the worker address with the given socket address.
   * @param socket_address The socket address to set for the worker.
   */
  void set_worker_address(std::string socket_address);

  /**
   * @brief Sets the agent address with the given agent address.
   * @param agent_address The agent address to set for the agent.
   */
  void set_agent_address(std::string agent_address);

  /**
   * @brief Returns the agent address.
   * @return A reference to the agent address string.
   */
  [[nodiscard]] std::string_view get_agent_address() const;

private:
  std::shared_ptr<ArmoniK::Api::Common::options::ComputePlane> impl;
};

class IConfiguration {
public:
  /**
   * @brief Default constructor.
   */
  IConfiguration();

  /**
   * @brief Default virtual destructor.
   */
  virtual ~IConfiguration();

  /**
   * @brief Get the value associated with the given key.
   * @param string Key to look up.
   * @return The value associated with the key, as a string.
   */
  [[nodiscard]] std::string get(const std::string &string) const;

  /**
   * @brief Set the value associated with the given key.
   * @param string Key to set the value for.
   * @param value Value to set for the key.
   */
  void set(const std::string &string, const std::string &value);

  /**
   * @brief Set the values from another IConfiguration object.
   * @param other IConfiguration object to copy values from.
   */
  void set(const IConfiguration &other);

  /**
   * @brief List defined values of this configuration.
   * @note Does not include environment variables
   */
  [[nodiscard]] const std::map<std::string, std::string> &list() const;

  /**
   * @brief Add JSON configuration from a file.
   * @param file_path Path to the JSON file.
   * @return Reference to the current IConfiguration object.
   */
  IConfiguration &add_json_configuration(std::string_view file_path);

  /**
   * @brief Add environment variable configuration.
   * @return Reference to the current IConfiguration object.
   */
  IConfiguration &add_env_configuration();

  /**
   * @brief Get the current ComputePlane configuration.
   * @return A ComputePlane object representing the current configuration.
   */
  [[nodiscard]] ComputePlane get_compute_plane() const;

  /**
   * @brief Get the current ControlPlane configuration
   * @return A ControlPlane object
   */
  [[nodiscard]] ControlPlane get_control_plane() const;

private:
  std::shared_ptr<ArmoniK::Api::Common::utils::IConfiguration> impl;
  friend class ComputePlane;
  friend class ControlPlane;
};
} // namespace SDK_COMMON_NAMESPACE

#endif // ARMONIK_SDK_ICONFIGURATION_H
