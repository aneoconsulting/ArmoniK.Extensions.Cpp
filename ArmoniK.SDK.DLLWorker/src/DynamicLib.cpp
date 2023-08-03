#include "DynamicLib.h"
#include <ArmoniKSdkException.h>
#include <dlfcn.h>
#include <stdexcept>

namespace SDK_DLLWORKER_NAMESPACE {

DynamicLib::DynamicLib(const char *filename) {
#ifdef LM_ID_NEWLM
#ifdef NDEBUG
  auto constexpr openmode = LM_ID_NEWLM;
#else
  auto constexpr openmode = LM_ID_BASE;
#endif
  handle = dlmopen(openmode, filename, RTLD_NOW | RTLD_LOCAL);
#else
  handle = dlopen(filename, RTLD_NOW | RTLD_LOCAL);
#endif
  if (!handle) {
    throw ArmoniK::Sdk::Common::ArmoniKSdkException(std::string("Could not dlopen ") + filename + " : " + dlerror());
  }
}

/**
 * @brief Destructor
 */
DynamicLib::~DynamicLib() { unload(); }

/**
 * @brief Unload the library
 */
void DynamicLib::unload() {
  if (!handle) {
    return;
  }
  if (dlclose(handle) == 0) {
    handle = nullptr;
  } else {
    throw ArmoniK::Sdk::Common::ArmoniKSdkException("Could not dlclose");
  }
}

/**
 * @brief Retrieve symbol from lib
 */
void *DynamicLib::get(const char *symbol_name) const {
  if (!handle) {
    throw ArmoniK::Sdk::Common::ArmoniKSdkException("Was not dlopen'ed");
  }
  void *sym = dlsym(handle, symbol_name);

  if (!sym) {
    throw ArmoniK::Sdk::Common::ArmoniKSdkException(std::string("Could not find symbol in lib : ") + symbol_name);
  }

  return sym;
}

} // namespace SDK_DLLWORKER_NAMESPACE
