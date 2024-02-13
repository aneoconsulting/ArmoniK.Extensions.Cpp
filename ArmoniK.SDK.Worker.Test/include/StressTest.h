//
// Created by fdenef on 01/02/2024.
//

#pragma once

#include <algorithm>
#include <armonik/sdk/worker/ServiceBase.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <numeric>
#include <vector>

namespace ArmoniK {
namespace Sdk {
namespace Worker {
namespace Test {

class StressTest final : public ServiceBase {
public:
  void *enter_session(const char *session_id) final {
    std::cout << "StressTest enter session: " << session_id << '\n';
    return new std::string(session_id);
  }
  void leave_session(void *session_ctx) final {
    const auto session_id = static_cast<std::string *>(session_ctx);
    std::cout << "StressTest leaving session: " << session_id << '\n';
    delete session_id;
  }
  std::string call(void *, const std::string &name, const std::string &input) final {
    if (name == "compute_workload") {
      const auto resultWorkload = [&]() -> std::vector<double> {
        const std::size_t sizeofNbOutput = sizeof(std::uint32_t);
        std::uint32_t nbOutputBytes = 0;
        std::uint32_t workLoadTimeInMs = 0;
        std::uint32_t nbInputBytes = 0;

        auto beginPtr = input.data();
        std::memcpy(&nbOutputBytes, beginPtr, sizeofNbOutput);

        beginPtr += sizeofNbOutput;
        std::memcpy(&workLoadTimeInMs, beginPtr, sizeofNbOutput);

        beginPtr += sizeofNbOutput;
        std::memcpy(&nbInputBytes, beginPtr, sizeofNbOutput);

        beginPtr += sizeofNbOutput;
        std::vector<double> inputWorkload(nbInputBytes, 0.0);
        std::memcpy(inputWorkload.data(), beginPtr, nbInputBytes);
        return compute_workload(inputWorkload, nbOutputBytes, workLoadTimeInMs);
      }();
      const auto resultSize = resultWorkload.size();
      std::vector<char> result;
      result.resize(sizeof(double) * resultSize, 0);
      std::memcpy(result.data(), resultWorkload.data(), sizeof(double) * resultSize);
      return {result.data(), result.size()};
    }
    throw std::runtime_error("Unknown method name: " + name);
  }
  ~StressTest() final = default;

  std::vector<double> compute_workload(const std::vector<double> &input, const std::size_t nbOutputBytes,
                                       const std::uint32_t workLoadTimeInMs) noexcept {
    using clock = std::chrono::high_resolution_clock;
    using milliseconds = std::chrono::milliseconds;

    if (input.empty() || nbOutputBytes <= 0) {
      return std::vector<double>{};
    }

    std::vector<double> output(nbOutputBytes / 8, 0);
    const std::size_t output_size = output.size();
    const auto double_output_size = static_cast<double>(output_size);

    double result = 0.;
    for (auto x : input) {
      result += x * x * x;
    }
    const auto end = clock::now() + milliseconds(workLoadTimeInMs);
    while (clock::now() <= end) {
      for (std::size_t i = 0; i < output_size; ++i) {
        output[i] = result / double_output_size;
      }
    }
    return output;
  }
};

} // namespace Test
} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
