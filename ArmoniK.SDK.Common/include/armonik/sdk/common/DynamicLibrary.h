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
  static constexpr const char *KeyConventionVersion = "ConventionVersion";
  static constexpr const char *KeyLibraryPath = "LibraryPath";
  static constexpr const char *KeySymbol = "Symbol";
  // Blob ID of an uploaded library archive (raw .so). When set, the worker downloads the library
  // from ArmoniK blob storage at task execution time instead of reading it from the local filesystem.
  // LibraryPath is not required when LibraryBlobId is set.
  static constexpr const char *KeyLibraryBlobId = "LibraryBlobId";
  static constexpr const char *ConventionVersion = "v1";

  // Path to the .so to load on the worker filesystem.
  // Not required when library_blob_id is set (the path is resolved at runtime from the blob).
  std::string library_path;

  /* Name of the method to call in the library (passed as function_name to armonik_call).
   * Required in convention mode when the payload does not carry a method name. */
  std::string symbol;

  // Blob ID of an uploaded .so file. When non-empty the worker fetches the library content
  // from the task's data dependencies and writes it to a temp file before dlopen-ing it.
  // Use SessionService::UploadLibrary() to obtain this ID.
  std::string library_blob_id;
};

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
