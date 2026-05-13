#include "armonik/sdk/common/TaskPayload.h"
#include "armonik/sdk/common/internal/ConventionPayload.h"
#include "armonik/sdk/common/ArmoniKSdkException.h"
#include <cstdint>
#include <iomanip>
#include <nlohmann/json.hpp>
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

template <typename T> T hex_to_int(absl::string_view str) {
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

absl::string_view advance_sv(absl::string_view &sv, size_t offset) {
  absl::string_view extracted = sv.substr(0, offset);
  sv = sv.substr(offset);
  return extracted;
}
} // namespace

// ---------------------------------------------------------------------------
// TaskPayload (legacy binary format)
// ---------------------------------------------------------------------------

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

std::string TaskPayload::Serialize() const {
  std::stringstream ss;
  ss << int_to_hex((field_size_t)method_name.size()) << method_name << int_to_hex((field_size_t)arguments.size());
  ss.write(arguments.data(), arguments.size());
  for (auto &&dd : data_dependencies) {
    ss << int_to_hex((field_size_t)dd.size()) << dd;
  }
  return ss.str();
}

TaskPayload TaskPayload::Deserialize(absl::string_view serialized) {
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

#pragma GCC diagnostic pop

// ---------------------------------------------------------------------------
// ConventionPayload (JSON wire format for the convention path)
// ---------------------------------------------------------------------------

std::string ConventionPayload::Serialize() const {
  nlohmann::json j;
  j["method"] = method_name;
  j["inputs"] = inputs;
  j["outputs"] = outputs;
  return j.dump();
}

ConventionPayload ConventionPayload::Deserialize(absl::string_view serialized) {
  try {
    auto j = nlohmann::json::parse(serialized.begin(), serialized.end());
    ConventionPayload payload;
    payload.method_name = j.value("method", std::string{});
    payload.inputs = j.at("inputs").get<std::map<std::string, std::string>>();
    payload.outputs = j.at("outputs").get<std::map<std::string, std::string>>();
    return payload;
  } catch (const nlohmann::json::exception &e) {
    throw ArmoniKSdkException(std::string("Failed to deserialize convention payload JSON: ") + e.what());
  }
}

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
