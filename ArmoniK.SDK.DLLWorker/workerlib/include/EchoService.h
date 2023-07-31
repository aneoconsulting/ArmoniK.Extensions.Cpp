#ifndef ARMONIK_SDK_ECHOSERVICE_H
#define ARMONIK_SDK_ECHOSERVICE_H

#include "ServiceBase.h"
#include <iostream>
namespace End2EndTest {

class EchoService : ServiceBase {
public:
  std::string call(void *session_ctx, const std::string &name, const std::string &input) override {
    std::cout << "EchoService method : " << name << std::endl;
    return input;
  }

  void *enter_session(const char *session_id) override {
    std::cout << "EchoService entering session : " << session_id << std::endl;
    return new std::string(session_id);
  }
  void leave_session(void *session_ctx) override {
    auto session_id = static_cast<std::string *>(session_ctx);
    std::cout << "EchoService leaving session : " << *session_id << std::endl;
    delete session_id;
  }
  ~EchoService() override { std::cout << "Deleted EchoService" << std::endl; };

  EchoService() : ServiceBase() { std::cout << "Created EchoService" << std::endl; }
};

} // namespace End2EndTest

#endif // ARMONIK_SDK_ECHOSERVICE_H
