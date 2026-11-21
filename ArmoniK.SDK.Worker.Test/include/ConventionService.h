#pragma once

#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <armonik/sdk/common/BlobDefinition.h>
#include <armonik/sdk/common/TaskDefinition.h>
#include <armonik/sdk/worker/ServiceBase.h>
#include <armonik/sdk/worker/TaskContext.h>
#include <chrono>
#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>
#include <thread>

namespace ArmoniK {
namespace Sdk {
namespace Worker {
namespace Test {

class ConventionService : public ServiceBase {
public:
  std::string call(void *, const std::string &name, const std::map<std::string, std::string> &inputs) override {
    if (name == "square") {
      int x = std::stoi(inputs.at("x"));
      return std::to_string(x * x);
    }
    if (name == "add") {
      int a = std::stoi(inputs.at("a"));
      int b = std::stoi(inputs.at("b"));
      return std::to_string(a + b);
    }
    if (name == "hex") {
      // The bytes of the input, which need not be valid UTF-8
      std::string hex;
      for (const unsigned char c : inputs.at("x")) {
        char digits[3];
        std::snprintf(digits, sizeof(digits), "%02x", c);
        hex += digits;
      }
      return hex;
    }
    if (name == "sleep_ms") {
      std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(inputs.at("ms"))));
      return "late";
    }
    throw std::runtime_error("ConventionService: unknown method: " + name);
  }

  // Methods with several outputs or subtasks use the task context. The others go through the default, which
  // calls the single-output overload above.
  void process(void *session_ctx, const std::string &name, TaskContext &context) override {
    if (name == "divmod" || name == "quotient_only") {
      int a = std::stoi(context.input("a").data());
      int b = std::stoi(context.input("b").data());
      context.output("quotient").write(std::to_string(a / b));
      if (name == "divmod") {
        context.output("remainder").write(std::to_string(a % b));
      }
      // quotient_only omits the remainder: the task must fail
      return;
    }
    if (name == "count" || name == "count_fail") {
      // HtcMock-style tree: count(n) = count(n - 1) + count(n - 1) = 2^n, with the sum taking over the output.
      // The leaves of count_fail fail, so the output of the root task is aborted.
      int n = std::stoi(context.input("n").data());
      if (n == 0) {
        if (name == "count_fail") {
          throw Common::ArmoniKSdkException("count_fail: leaf failed");
        }
        context.output("result").write("1");
        return;
      }
      using Common::BlobDefinition;
      using Common::TaskDefinition;
      const auto halves = context.create_blobs_metadata({"left", "right"});
      const auto child = BlobDefinition::FromData(std::to_string(n - 1));
      context.submit_tasks({TaskDefinition().WithInput("n", child).WithOutput("result", halves[0]),
                            TaskDefinition().WithInput("n", child).WithOutput("result", halves[1])});
      context.submit_tasks({TaskDefinition()
                                .WithInput("a", BlobDefinition::FromBlobId(halves[0]))
                                .WithInput("b", BlobDefinition::FromBlobId(halves[1]))
                                .WithOutput("result", context.output("result").id())},
                           context.subtask_options("add"));
      return;
    }
    if (name == "first_then_count") {
      // Two outputs produced at different times: "first" now, "second" by a count(n) subtree, for WaitBehavior::Any
      context.output("first").write("1");
      context.submit_tasks({Common::TaskDefinition()
                                .WithInput("n", context.input("n").AsBlobDefinition())
                                .WithOutput("result", context.output("second").id())},
                           context.subtask_options("count"));
      return;
    }
    ServiceBase::process(session_ctx, name, context);
  }

  void *enter_session(const char *session_id) override { return new std::string(session_id); }
  void leave_session(void *session_ctx) override { delete static_cast<std::string *>(session_ctx); }
};

} // namespace Test
} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
