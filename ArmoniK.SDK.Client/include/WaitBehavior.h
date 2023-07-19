#ifndef ARMONIK_SDK_WAITBEHAVIOR_H
#define ARMONIK_SDK_WAITBEHAVIOR_H

namespace SDK_CLIENT_NAMESPACE{
struct WaitOptions{
  unsigned int polling_ms = 500;
};

enum WaitBehavior{
  All = 0,
  Any = 1,
  BreakOnError = 2
};

inline enum WaitBehavior operator|(WaitBehavior a, WaitBehavior b){
      return static_cast<WaitBehavior>(static_cast<int>(a)|static_cast<int>(b));
}

}

#endif // ARMONIK_SDK_WAITBEHAVIOR_H
