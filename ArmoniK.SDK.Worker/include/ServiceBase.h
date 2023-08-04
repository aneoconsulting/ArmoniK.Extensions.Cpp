#ifndef ARMONIK_SDK_SERVICEBASE_H
#define ARMONIK_SDK_SERVICEBASE_H

#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Worker {
class ServiceBase {
public:
  virtual void *enter_session(const char *session_id) {
    (void)session_id;
    return nullptr;
  }
  virtual void leave_session(void *session_ctx) { (void)session_ctx; }

  virtual std::string call(void *session_ctx, const std::string &name, const std::string &input) = 0;
  virtual ~ServiceBase() = default;
};
} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK

#endif // ARMONIK_SDK_SERVICEBASE_H
