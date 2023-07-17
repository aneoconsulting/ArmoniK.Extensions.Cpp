#ifndef ARMONIK_SDK_CHANNELPOOL_H
#define ARMONIK_SDK_CHANNELPOOL_H

#include "Properties.h"
#include <grpcpp/channel.h>

class ChannelPool {
public:
  explicit ChannelPool(ArmoniK::SDK::Common::Properties properties);

  std::shared_ptr<grpc::Channel> GetChannel();

private:
  ArmoniK::SDK::Common::Properties properties_;
};

#endif // ARMONIK_SDK_CHANNELPOOL_H
