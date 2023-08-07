#ifndef ARMONIK_SDK_ARMONIKSDKEXCEPTION_H
#define ARMONIK_SDK_ARMONIKSDKEXCEPTION_H

#include <stdexcept>
namespace SDK_COMMON_NAMESPACE {
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
} // namespace SDK_COMMON_NAMESPACE

#endif // ARMONIK_SDK_ARMONIKSDKEXCEPTION_H
