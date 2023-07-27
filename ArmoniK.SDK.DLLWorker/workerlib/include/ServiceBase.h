#ifndef ARMONIK_SDK_SERVICEBASE_H
#define ARMONIK_SDK_SERVICEBASE_H

#include <string>

class ServiceBase {
public:
  virtual void *enter_session(const char *session_id) { return nullptr; }
  virtual void leave_session(void *session_ctx) {}

  virtual std::string call(void *session_ctx, const std::string &name, std::string_view input) = 0;
  virtual ~ServiceBase() = default;
};

#endif // ARMONIK_SDK_SERVICEBASE_H
