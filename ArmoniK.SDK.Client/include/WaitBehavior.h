#ifndef ARMONIK_SDK_WAITBEHAVIOR_H
#define ARMONIK_SDK_WAITBEHAVIOR_H

namespace SDK_CLIENT_NAMESPACE {
struct WaitOptions {
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

#endif // ARMONIK_SDK_WAITBEHAVIOR_H