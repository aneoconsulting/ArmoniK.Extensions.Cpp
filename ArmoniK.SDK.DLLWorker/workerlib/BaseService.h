#pragma once

#include <string>

class BaseService {
private:
  /* data */

public:
  BaseService(/* args */);
  virtual ~BaseService() = default;

  virtual void *create_service(void *service_ctx);

  virtual void *enter_session(const char *session_id);

  virtual void leave_session(void *session_ctx);

  virtual std::string call(std::string name, std::string input) = 0;

  virtual void destroy_service(void *service_ctx);
};
