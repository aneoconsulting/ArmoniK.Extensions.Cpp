#pragma once
#include <utility>
namespace SDK_DLLWORKER_NAMESPACE {
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
   * DynamicLib poits to no library
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
   */
  DynamicLib(DynamicLib &&other) noexcept : handle(other.handle) { other.handle = nullptr; }

  /**
   * @brief Move assignment operator
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
   */
  void *get(const char *symbol_name) const;

  /**
   * @brief Retrieve symbol from lib
   */
  template <class T> T get(const char *symbol_name) const { return (T)(get(symbol_name)); }

  /**
   * @brief Test whether a library is loaded or not
   */
  explicit operator bool() const noexcept { return handle; }
};
} // namespace SDK_DLLWORKER_NAMESPACE
