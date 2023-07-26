#include "TaskPayload.h"
#include <charconv>
#include <iomanip>
#include <string>

namespace SDK_COMMON_NAMESPACE {

/**
 * @brief Type used to define the size of a field in the payload
 */
typedef uint32_t field_size_t;

template <typename T> struct TypeParseTraits;

#define REGISTER_PARSE_TYPE(X)                                                                                         \
  template <> struct TypeParseTraits<X> { static const char *name; };                                                  \
  const char *TypeParseTraits<X>::name = #X

REGISTER_PARSE_TYPE(uint32_t);

/**
 * @brief Converts an integer type to a hex string
 * @tparam T Integer type
 * @param i Integer
 * @return Hex string of this integer
 */
template <typename T> std::string int_to_hex(T i) {
  std::stringstream stream;
  stream << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
  return stream.str();
}

/**
 * @brief Converts a hex string into a integer type
 * @tparam T Integer type
 * @param str Hex string
 * @return integer obtained from the string
 * @throws std::runtime_error if the hex string is invalid or if it's too large for the given type
 */
template <typename T> T hex_to_int(std::string_view str) {
  T value;
  auto result = std::from_chars(str.data(), str.data() + str.size(), value, 16);
  if (result.ec == std::errc::invalid_argument) {
    throw std::runtime_error(std::string(str) + " is not convertible to " + TypeParseTraits<T>::name);
  }
  if (result.ec == std::errc::result_out_of_range) {
    throw std::runtime_error(std::string(str) + " is too large for " + TypeParseTraits<T>::name);
  }
  return value;
}

std::string TaskPayload::Serialize() const {
  std::stringstream ss;
  // Method name
  ss << int_to_hex((field_size_t)method_name.size()) << method_name << int_to_hex((field_size_t)arguments.size());

  // Writing directly to avoid stopping at null character
  ss.write(arguments.data(), arguments.size());

  // Data dependencies
  for (auto &&dd : data_dependencies) {
    ss << int_to_hex((field_size_t)dd.size()) << dd;
  }
  return ss.str();
}
TaskPayload TaskPayload::Deserialize(std::string_view serialized) {
  constexpr uint32_t size_width = sizeof(field_size_t) * 2;
  field_size_t fieldSize;
  uint32_t position = 0;
  std::vector<std::string> data_dependencies;

  // Method name
  fieldSize = hex_to_int<field_size_t>(serialized.substr(position, size_width));
  position += size_width;
  std::string method_name(serialized.data() + position, fieldSize);
  position += fieldSize;

  // Method arguments
  fieldSize = hex_to_int<field_size_t>(serialized.substr(position, size_width));
  position += size_width;
  std::string arguments(serialized.data() + position, fieldSize);
  position += fieldSize;

  // Data dependencies
  while (position < serialized.size()) {
    fieldSize = hex_to_int<field_size_t>(serialized.substr(position, size_width));
    position += size_width;
    data_dependencies.emplace_back(serialized.data() + position, fieldSize);
    position += fieldSize;
  }

  return {method_name, arguments, data_dependencies};
}
} // namespace SDK_COMMON_NAMESPACE