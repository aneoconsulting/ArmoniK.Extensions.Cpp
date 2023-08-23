#pragma once

#include <armonik/sdk/common/Properties.h>
#include <grpcpp/channel.h>
#include <queue>
#include <shared_mutex>

namespace SDK_CLIENT_NAMESPACE::Internal {

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
  ChannelPool(const ChannelPool &) = delete;

  ChannelPool(ChannelPool &&other) noexcept;
  ChannelPool &operator=(const ChannelPool &) = delete;

  ChannelPool &operator=(ChannelPool &&other) noexcept;
  ~ChannelPool();

  /**
   * @brief
   *
   * @return std::shared_ptr<grpc::Channel>
   */
  std::shared_ptr<grpc::Channel> AcquireChannel();

  /**
   * @brief Releases a channel to the pool
   *
   * @param channel
   */
  void ReleaseChannel(std::shared_ptr<grpc::Channel> channel);

  /**
   * @brief Check the state of a channel and shut it down in case of failure
   *
   * @param channel
   * @return true if channel has been shut down
   * @return false if not
   */
  static bool ShutdownOnFailure(std::shared_ptr<grpc::Channel> channel);

  std::shared_ptr<grpc::Channel> WithChannel();

  class ChannelGuard {
  public:
    std::shared_ptr<grpc::Channel> channel;

    ChannelGuard(const ArmoniK::Sdk::Common::Properties &properties);

    void Dispose();

  private:
    std::shared_ptr<ChannelPool> pool_;
  };

  ChannelGuard GetChannel();

private:
  ArmoniK::Sdk::Common::Properties properties_;
  std::queue<std::shared_ptr<grpc::Channel>> channel_pool_;
  std::shared_mutex channel_mutex_;
};

} // namespace SDK_CLIENT_NAMESPACE::Internal
