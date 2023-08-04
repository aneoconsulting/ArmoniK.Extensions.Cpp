#ifndef ARMONIK_SDK_CHANNELPOOL_H
#define ARMONIK_SDK_CHANNELPOOL_H

#include "Properties.h"
#include <grpcpp/channel.h>

namespace SDK_CLIENT_NAMESPACE {

/**
 * @brief A pool for Grpc channels
 */
class ChannelPool {
public:
  /**
   * @brief Creates a channel pool from the given properties
   * @param properties Properties
   */
  explicit ChannelPool(ArmoniK::Sdk::Common::Properties properties);

  /**
   * @brief Gets or creates a channel from the pool
   * @return grpc channel
   */
  std::shared_ptr<grpc::Channel> GetChannel();

private:
  ArmoniK::Sdk::Common::Properties properties_;
};

} // namespace SDK_CLIENT_NAMESPACE

#endif // ARMONIK_SDK_CHANNELPOOL_H
