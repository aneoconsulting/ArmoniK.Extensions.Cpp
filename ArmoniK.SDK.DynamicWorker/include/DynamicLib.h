#pragma once

#include <utility>

namespace ArmoniK {
namespace Sdk {
namespace DynamicWorker {
/**
 * @brief Manage external dynamic library
 */
class DynamicLib {
private:
  /**
   * @brief Native handle to the dlopen'ed library
   */
  void *handle = nullptr;

public:
  /**
   * @brief Default constructor
   *
   * DynamicLib points to no library
   */
  DynamicLib() noexcept = default;

  /**
   * @brief Load a library constructor
   *
   * @param filename path to the library to load
   */
  explicit DynamicLib(const char *filename);

  /**
   * @brief Deleted copy constructor
   */
  DynamicLib(const DynamicLib &) = delete;

  /**
   * @brief Deleted copy assignment operator
   */
  DynamicLib &operator=(const DynamicLib &) = delete;

  /**
   * @brief Move constructor
   * @param other Other DynamicLib
   */
  DynamicLib(DynamicLib &&other) noexcept : handle(other.handle) { other.handle = nullptr; }

  /**
   * @brief Move assignment operator
   * @param other Other DyncmicLib
   * @return this
   */
  DynamicLib &operator=(DynamicLib &&other) noexcept {
    std::swap(handle, other.handle);
    return *this;
  }

  /**
   * @brief Destructor
   */
  ~DynamicLib();

  /**
   * @brief Unload the library
   */
  void unload();

  /**
   * @brief Retrieve symbol from lib
   * @param symbol_name Name of the symbol
   * @return Function pointer for the requested symbol
   */
  void *get(const char *symbol_name) const;

  /**
   * @brief Retrieve symbol from lib
   * @tparam T Function pointer type
   * @param symbol_name Name of the symbol
   * @return Function pointer for the requested symbol
   */
  template <class T> T get(const char *symbol_name) const { return (T)get(symbol_name); }

  /**
   * @brief Retrieve an optional symbol from lib
   * @tparam T Function pointer type
   * @param symbol_name Name of the symbol
   * @return Function pointer for the requested symbol, null if the library does not export it
   */
  template <class T> T try_get(const char *symbol_name) const { return (T)try_get(symbol_name); }

  /**
   * @brief Retrieve an optional symbol from lib
   * @param symbol_name Name of the symbol
   * @return Pointer to the requested symbol, null if the library does not export it
   */
  void *try_get(const char *symbol_name) const;

  /**
   * @brief Test whether a library is loaded or not
   * @return true if a library is loaded
   */
  explicit operator bool() const noexcept { return handle; }
};
} // namespace DynamicWorker
} // namespace Sdk
} // namespace ArmoniK
