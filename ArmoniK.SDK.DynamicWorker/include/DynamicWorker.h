#pragma once

#include "ApplicationManager.h"
#include <armonik/common/logger/local_logger.h>
#include <armonik/sdk/common/Configuration.h>
#include <armonik/worker/Worker/ArmoniKWorker.h>
#include <cstddef>
#include <string>

namespace ArmoniK {
namespace Sdk {
namespace DynamicWorker {
/**
 * @brief ArmoniK Worker that loads a dynamic library and executes method within it
 */
class DynamicWorker : public armonik::api::worker::ArmoniKWorker {
public:
  /**
   * @brief Creates a dynamic worker
   * @param agent Stub to communicate with the scheduling agent
   * @param config Configuration
   */
  explicit DynamicWorker(std::unique_ptr<armonik::api::grpc::v1::agent::Agent::Stub> agent,
                         const ArmoniK::Sdk::Common::Configuration &config,
                         const armonik::api::common::logger::Logger &logger);

  /**
   * @brief Executes the task given by the task handler
   * @param taskHandler Task handler
   * @return Whether the task executed successfully or not
   */
  armonik::api::worker::ProcessStatus Execute(armonik::api::worker::TaskHandler &taskHandler) override;

  /**
   * @brief Removes the libraries written by this worker
   */
  ~DynamicWorker();

private:
  /**
   * @brief Writes a library blob to this worker's private directory, unless it is already there
   * @param blob_id Id of the library blob
   * @param data Content of the library
   * @param size Size of the content
   * @return Path to the library file
   */
  std::string InstallLibrary(const std::string &blob_id, const char *data, std::size_t size);

  /**
   * @brief Private directory where the library blobs are written, created on first use
   */
  std::string library_dir_;

  /**
   * @brief Local logger
   */
  armonik::api::common::logger::LocalLogger logger;

  /**
   * @brief Application manager
   */
  ArmoniK::Sdk::DynamicWorker::ApplicationManager manager;
};
} // namespace DynamicWorker
} // namespace Sdk
} // namespace ArmoniK
