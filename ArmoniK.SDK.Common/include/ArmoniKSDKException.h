#ifndef ARMONIK_SDK_ARMONIKSDKEXCEPTION_H
#define ARMONIK_SDK_ARMONIKSDKEXCEPTION_H

#include <stdexcept>
namespace SDK_COMMON_NAMESPACE {
class ArmoniKSDKException : public std::runtime_error {
public:
  explicit ArmoniKSDKException(const char *message) : std::runtime_error(message) {}
  explicit ArmoniKSDKException(const std::string &message) : std::runtime_error(message) {}
};
} // namespace SDK_COMMON_NAMESPACE

#endif // ARMONIK_SDK_ARMONIKSDKEXCEPTION_H
