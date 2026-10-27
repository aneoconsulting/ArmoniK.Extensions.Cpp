#include "armonik/sdk/common/TaskPayload.h"
#include "armonik/sdk/common/ArmoniKSdkException.h"
#include "armonik/sdk/common/Diagnostics.h"
#include "armonik/sdk/common/TaskDefinition.h"
#include <cstdint>
#include <iomanip>
#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Common {

// ---------------------------------------------------------------------------
// Legacy binary format helpers
// ---------------------------------------------------------------------------

typedef uint32_t field_size_t;

namespace {
template <typename T> struct TypeParseTraits;

#define REGISTER_PARSE_TYPE(X)                                                                                         \
  template <> struct TypeParseTraits<X> {                                                                              \
    static const char *name;                                                                                           \
  };                                                                                                                   \
  const char *TypeParseTraits<X>::name = #X

REGISTER_PARSE_TYPE(uint32_t);

template <typename T> std::string int_to_hex(T i) {
  std::stringstream stream;
  stream << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
  return stream.str();
}

template <typename T> T hex_to_int(armonik::api::string_view str) {
  char *endPos;
  std::string null_terminated(str.data(), str.size());
  T result = std::strtol(null_terminated.data(), &endPos, 16);
  if (errno == ERANGE) {
    throw std::runtime_error(null_terminated + " is too large for " + TypeParseTraits<T>::name);
  }
  if (endPos != null_terminated.data() + null_terminated.size()) {
    throw std::runtime_error(null_terminated + " is not convertible to " + TypeParseTraits<T>::name);
  }
  return result;
}

armonik::api::string_view advance_sv(armonik::api::string_view &sv, size_t offset) {
  armonik::api::string_view extracted = sv.substr(0, offset);
  sv = sv.substr(offset);
  return extracted;
}
} // namespace

// ---------------------------------------------------------------------------
// TaskPayload (legacy binary format)
// ---------------------------------------------------------------------------

ARMONIK_SDK_SUPPRESS_DEPRECATED_BEGIN

std::string TaskPayload::Serialize() const {
  std::stringstream ss;
  ss << int_to_hex((field_size_t)method_name.size()) << method_name << int_to_hex((field_size_t)arguments.size());
  ss.write(arguments.data(), arguments.size());
  for (auto &&dd : data_dependencies) {
    ss << int_to_hex((field_size_t)dd.size()) << dd;
  }
  return ss.str();
}

TaskPayload TaskPayload::Deserialize(armonik::api::string_view serialized) {
  constexpr size_t size_width = sizeof(field_size_t) * 2;
  field_size_t fieldSize;
  std::vector<std::string> data_dependencies;

  fieldSize = hex_to_int<field_size_t>(advance_sv(serialized, size_width));
  std::string method_name(advance_sv(serialized, fieldSize));

  fieldSize = hex_to_int<field_size_t>(advance_sv(serialized, size_width));
  std::string arguments(advance_sv(serialized, fieldSize));

  while (!serialized.empty()) {
    fieldSize = hex_to_int<field_size_t>(advance_sv(serialized, size_width));
    data_dependencies.emplace_back(advance_sv(serialized, fieldSize));
  }

  return {std::move(method_name), std::move(arguments), std::move(data_dependencies)};
}

ARMONIK_SDK_SUPPRESS_DEPRECATED_END

constexpr const char *TaskDefinition::DefaultOutputName;

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
