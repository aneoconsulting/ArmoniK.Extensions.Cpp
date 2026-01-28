#pragma once

#include <absl/strings/string_view.h>
#include <armonik/common/logger/fwd.h>
#include <map>
#include <memory>
#include <string>

/**
 * @brief This file declares proxy objects for the Configuration objects
 */

namespace armonik {
namespace api {
namespace common {
namespace utils {
class Configuration;
}
namespace options {
class ComputePlane;
class ControlPlane;
} // namespace options
} // namespace common
} // namespace api
} // namespace armonik

namespace ArmoniK {
namespace Sdk {
namespace Common {
class Configuration;

/**
 * @brief Control plane connectivity configuration
 */
class ControlPlane {
public:
  /**
   * @brief Constructs a ControlPlane object with the given configuration.
   * @param config The Configuration object containing address information.
   */
  ControlPlane(const Configuration &config);

  /**
   * @brief Copy constructor
   * @param other Other Control Plane
   */
  ControlPlane(const ControlPlane &other);

  /**
   * @brief Move constructor
   * @param other Other Control Plane
   */
  ControlPlane(ControlPlane &&other) noexcept;

  /**
   * @brief Copy assignment operator
   * @param other Other Control Plane
   * @return this
   */
  ControlPlane &operator=(const ControlPlane &other);

  /**
   * @brief Move assignment operator
   * @param other Other Control Plane
   * @return this
   */
  ControlPlane &operator=(ControlPlane &&other) noexcept;

  /**
   * @brief Destroy the ControlPlane object
   */
  ~ControlPlane();

  /**
   * @brief ArmoniK control plane endpoint
   * @return Endpoint address
   */
  [[nodiscard]] absl::string_view getEndpoint() const;

  /**
   * @brief Path to the client's certificate in PEM format
   * @return Client certificate's path
   */
  [[nodiscard]] absl::string_view getUserCertPemPath() const;

  /**
   * @brief Path to the client's key in PEM format (PKCS#1 or PKCS#8)
   * @return Client key path
   */
  [[nodiscard]] absl::string_view getUserKeyPemPath() const;

  /**
   * @brief Path to the client's PKCS#12 certificate/key
   * @return Client P12 path
   */
  [[nodiscard]] absl::string_view getUserP12Path() const;

  /**
   * @brief Path to the server's CA certificate
   * @return CA certificate path
   */
  [[nodiscard]] absl::string_view getCaCertPemPath() const;

  /**
   * @brief Is SSL validation enabled ?
   * @return True if SSL validation is enabled, false otherwise
   */
  [[nodiscard]] bool isSslValidation() const;

  /**
   * @brief Batch size for waiting results
   * @return Batch size
   */
  [[nodiscard]] int getWaitBatchSize() const;

  /**
   * @brief Batch size for task submission
   * @return Batch size
   */
  [[nodiscard]] int getSubmitBatchSize() const;

private:
  std::unique_ptr<armonik::api::common::options::ControlPlane> impl;
  [[nodiscard]] const armonik::api::common::options::ControlPlane &get_impl() const;
  armonik::api::common::options::ControlPlane &set_impl();
  int wait_batch_size_ = 200;
  int submit_batch_size_ = 200;
};

/**
 * @brief Compute plane connectivity configuration
 */
class ComputePlane {
public:
  /**
   * @brief Constructs a ComputePlane object with the given configuration.
   * @param configuration The Configuration object containing address information.
   */
  ComputePlane(const Configuration &configuration);

  /**
   * @brief Copy constructor
   * @param other Other compute plane
   */
  ComputePlane(const ComputePlane &other);

  /**
   * @brief Move constructor
   * @param other Other compute plane
   */
  ComputePlane(ComputePlane &&other) noexcept;

  /**
   * @brief Copy assignment operator
   * @param other Other compute plane
   * @return this
   */
  ComputePlane &operator=(const ComputePlane &other);

  /**
   * @brief Move assignment operator
   * @param other Other compute plane
   * @return this
   */
  ComputePlane &operator=(ComputePlane &&other) noexcept;

  /**
   * @brief Destroy the ComputePlane object
   */
  ~ComputePlane();

  /**
   * @brief Returns the server address.
   * @return A reference to the server address string.
   */
  [[nodiscard]] absl::string_view get_server_address() const;

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
  [[nodiscard]] absl::string_view get_agent_address() const;

private:
  std::unique_ptr<armonik::api::common::options::ComputePlane> impl;
  [[nodiscard]] const armonik::api::common::options::ComputePlane &get_impl() const;
  armonik::api::common::options::ComputePlane &set_impl();
};

/**
 * @brief Configuration
 */
class Configuration {
public:
  /**
   * @brief Default constructor.
   */
  Configuration();

  ~Configuration();

  /**
   * @brief Copy constructor
   * @param other Other configuration
   */
  Configuration(const Configuration &other);

  /**
   * @brief Move constructor
   * @param other Other configuration
   */
  Configuration(Configuration &&other) noexcept;

  /**
   * @brief Copy assignment operator
   * @param other Other configuration
   * @return this
   */
  Configuration &operator=(const Configuration &other);
  /**
   * @brief Move assignment operator
   * @param other Other configuration
   * @return this
   */
  Configuration &operator=(Configuration &&other) noexcept;

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
   * @brief List defined values of this configuration.
   * @return Map representation of this configuration
   * @note Does not include environment variables
   */
  [[nodiscard]] const std::map<std::string, std::string> &list() const;

  /**
   * @brief Add JSON configuration from a file.
   * @param file_path Path to the JSON file.
   * @return Reference to the current Configuration object.
   */
  Configuration &add_json_configuration(absl::string_view file_path);

  /**
   * @brief Add environment variable configuration.
   * @return Reference to the current Configuration object.
   */
  Configuration &add_env_configuration();

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

  /**
   * @brief Get the configured log level
   * @return Log level
   */
  [[nodiscard]] armonik::api::common::logger::Level get_log_level() const;

  /**
   * @brief Casts this configuration to its Api equivalent
   * @return Api Configuration equivalent to this
   */
  explicit operator armonik::api::common::utils::Configuration();

private:
  std::unique_ptr<armonik::api::common::utils::Configuration> impl;
  friend class ComputePlane;
  friend class ControlPlane;
  [[nodiscard]] const armonik::api::common::utils::Configuration &get_impl() const;
  armonik::api::common::utils::Configuration &set_impl();
};
} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
