#pragma once

namespace SDK_CLIENT_NAMESPACE {
/**
 * @brief Options for result waiting
 */
struct WaitOptions {
  /**
   * @brief Time in milliseconds for result status polling
   */
  unsigned int polling_ms = 500;
};

enum WaitBehavior {
  /**
   * Wait on all tasks completion
   */
  All = 0,
  /**
   * Wait on any task completion
   */
  Any = 1,
  /**
   * Stop waiting for all tasks if any one of them has an error
   */
  BreakOnError = 2
};

inline enum WaitBehavior operator|(WaitBehavior a, WaitBehavior b) {
  return static_cast<WaitBehavior>(static_cast<int>(a) | static_cast<int>(b));
}

} // namespace SDK_CLIENT_NAMESPACE
