#include "TaskRequest.h"
#include <charconv>
#include <iomanip>
#include <string>

namespace SDK_COMMON_NAMESPACE {

typedef uint32_t field_size_t;

template <typename T> std::string int_to_hex(T i) {
  std::stringstream stream;
  stream << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
  return stream.str();
}

template <typename T> T hex_to_int(std::string_view str) {
  T value;
  auto result = std::from_chars(str.data(), str.data() + str.size(), value);
  if (result.ec == std::errc::invalid_argument) {
    throw std::runtime_error(std::string(str) + " is not convertible to " + typeid(T).name());
  }
  if (result.ec == std::errc::result_out_of_range) {
    throw std::runtime_error(std::string(str) + " is too large for " + typeid(T).name());
  }
  return value;
}

std::string TaskRequest::Serialize() {
  std::stringstream ss;
  ss << int_to_hex((field_size_t)service_name.size()) << service_name << int_to_hex((field_size_t)method_name.size())
     << method_name << int_to_hex((field_size_t)arguments.size()) << arguments;

  for (auto &&dd : data_dependencies) {
    ss << int_to_hex((field_size_t)dd.size()) << dd;
  }
  return ss.str();
}
TaskRequest TaskRequest::Deserialize(std::string_view serialized) {
  constexpr uint32_t size_width = sizeof(field_size_t) * 2 + 2;
  field_size_t fieldSize;
  uint32_t position = 0;
  std::vector<std::string> data_dependencies;

  fieldSize = hex_to_int<field_size_t>(serialized.substr(position, size_width));
  position += size_width;
  std::string service_name(serialized.data() + position, fieldSize);
  position += fieldSize;

  fieldSize = hex_to_int<field_size_t>(serialized.substr(position, size_width));
  position += size_width;
  std::string method_name(serialized.data() + position, fieldSize);
  position += fieldSize;

  fieldSize = hex_to_int<field_size_t>(serialized.substr(position, size_width));
  position += size_width;
  std::string arguments(serialized.data() + position, fieldSize);
  position += fieldSize;

  while (position < serialized.size()) {
    fieldSize = hex_to_int<field_size_t>(serialized.substr(position, size_width));
    position += size_width;
    data_dependencies.emplace_back(serialized.data() + position, fieldSize);
    position += fieldSize;
  }

  return {service_name, method_name, arguments, data_dependencies};
}
} // namespace SDK_COMMON_NAMESPACE