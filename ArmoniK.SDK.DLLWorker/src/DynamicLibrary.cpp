#include <dlfcn.h>
#include <stdexcept>


namespace SDK_DLLWORKER_NAMESPACE {

DynamicLibrary::DynamicLib(const char* filename) {
  handle = dlmopen(LM_ID_NEWLM, filename, RTLD_NOW | RTLD_LOCAL);
  if (!handle) {
    throw std::runtime_error("Could not dlopen");
  }
}

/**
 * @brief Destructor
 */
DynamicLibrary::~DynamicLib() {
  unload();
}

/**
 * @brief Unload the library
 */
void DynamicLibrary::unload() {
  if (dlclose(handle) == 0) {
    handle = nullptr;
  } else {
    throw std::runtime_error("Could not dlclose");
  }
}

/**
 * @brief Retrieve symbol from lib
 */
void* DynamicLibrary::get(const char* symbol_name) const {
  if (!handle) {
    throw std::runtime_error("Was not dlopen'ed");
  }
  void* sym = dlsym(handle, symbol_name);

  if (!sym) {
    throw std::runtime_error("Could not find symbol in lib");
  }

  return sym;
}

} // namespace SDK_DLLWORKER_NAMESPACE
