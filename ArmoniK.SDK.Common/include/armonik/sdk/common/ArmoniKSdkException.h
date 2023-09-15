#pragma once

#include <stdexcept>
namespace ArmoniK {
namespace Sdk {
namespace Common {
/**
 * @brief ArmoniK SDK exception
 */
class ArmoniKSdkException : public std::runtime_error {
public:
  /**
   * @brief Creates an ArmoniKSdkException
   * @param message error message
   */
  explicit ArmoniKSdkException(const char *message) : std::runtime_error(message) {}
  /**
   * @brief Creates an ArmoniKSdkException
   * @param message error message
   */
  explicit ArmoniKSdkException(const std::string &message) : std::runtime_error(message) {}
};
} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
