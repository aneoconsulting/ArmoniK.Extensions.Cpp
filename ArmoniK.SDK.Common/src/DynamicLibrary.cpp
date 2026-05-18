#include <armonik/sdk/common/DynamicLibrary.h>

namespace ArmoniK {
namespace Sdk {
namespace Common {

// Out-of-class definitions required by C++14 for ODR-use of static constexpr members
constexpr const char *DynamicLibrary::KeyConventionVersion;
constexpr const char *DynamicLibrary::KeyLibraryPath;
constexpr const char *DynamicLibrary::KeySymbol;
constexpr const char *DynamicLibrary::KeyLibraryBlobId;
constexpr const char *DynamicLibrary::ConventionVersion;

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
