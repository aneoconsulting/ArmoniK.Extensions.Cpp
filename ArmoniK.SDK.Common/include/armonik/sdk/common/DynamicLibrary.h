#pragma once

#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Common {

/**
 * @brief Helper structure holding information to perform dynamic lib loading
 */
struct DynamicLibrary {
    // Keys used in TaskOptions.options 
    static constexpr const char* KeyConventionVersion = "ConventionVersion";
    static constexpr const char* KeyLibraryPath = "LibraryPath";
    static constexpr const char* KeySymbol = "Symbol";
    static constexpr const char* ConventionVersion = "v1";

    // Path to the .so to load
    std::string library_path;
    
    /* Optional prefix for armonik_* symbol names (e.g. "myapp" → "myapp_create_service")
     * Leave empty to use the default "armonik_" prefix */
    std::string symbol;
};

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
