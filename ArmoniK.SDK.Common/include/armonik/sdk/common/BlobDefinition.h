#pragma once

#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Common {

/**
 * @brief Represents an input blob: either raw data to be uploaded, or a reference to an
 * already-existing blob by ID.
 *
 * Use BlobDefinition::FromData() when you have the raw bytes and want the library to upload
 * them. Use BlobDefinition::FromBlobId() to reference a blob that was already uploaded (e.g.
 * a shared large dataset reused across many tasks).
 */
struct BlobDefinition {
  /**
   * @brief Creates a BlobDefinition carrying raw data to be uploaded on submission.
   * @param data Raw bytes to upload
   */
  static BlobDefinition FromData(std::string data) {
    BlobDefinition b;
    b.is_raw_data_ = true;
    b.value_ = std::move(data);
    return b;
  }

  /**
   * @brief Creates a BlobDefinition referencing an already-uploaded blob by its result ID.
   * @param blob_id Result ID of the existing blob
   */
  static BlobDefinition FromBlobId(std::string blob_id) {
    BlobDefinition b;
    b.is_raw_data_ = false;
    b.value_ = std::move(blob_id);
    return b;
  }

  /**
   * @brief Returns true if this definition holds raw data (to be uploaded), false if it
   * references an existing blob ID.
   */
  [[nodiscard]] bool IsRawData() const { return is_raw_data_; }

  /**
   * @brief Returns the raw data. Only valid when IsRawData() is true.
   */
  [[nodiscard]] const std::string &GetData() const { return value_; }

  /**
   * @brief Returns the blob ID. Only valid when IsRawData() is false.
   */
  [[nodiscard]] const std::string &GetBlobId() const { return value_; }

private:
  bool is_raw_data_ = true;
  std::string value_;
};

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
