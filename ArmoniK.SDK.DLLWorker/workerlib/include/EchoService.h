#ifndef ARMONIK_SDK_ECHOSERVICE_H
#define ARMONIK_SDK_ECHOSERVICE_H

#include "ServiceBase.h"
#include <iostream>
namespace End2EndTest {

class EchoService : ServiceBase {
  std::string call(void *session_ctx, const std::string &name, std::string_view input) override {
    std::cout << "EchoService method : " << name << std::endl;
    return std::string(input);
  }
};

} // namespace End2EndTest

#endif // ARMONIK_SDK_ECHOSERVICE_H
