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
        std::size_t nbOutputBytes = 0;
        std::uint32_t workLoadTimeInMs = 0;
        std::size_t sizeVector = 0;

        auto beginPtr = input.data();
        std::memcpy(&nbOutputBytes, beginPtr, sizeof(std::size_t));

        beginPtr += sizeof(std::size_t);
        std::memcpy(&workLoadTimeInMs, beginPtr, sizeof(std::uint32_t));

        beginPtr += sizeof(std::uint32_t);
        std::memcpy(&sizeVector, beginPtr, sizeof(std::size_t));

        beginPtr += sizeof(std::size_t);
        std::vector<double> inputWorkload(sizeVector, 0.0);
        std::memcpy(inputWorkload.data(), beginPtr, sizeVector * sizeof(double));
        return compute_workload(inputWorkload, nbOutputBytes, workLoadTimeInMs);
      }();
      const auto resultSize = resultWorkload.size();
      std::vector<char> result;
      result.resize(sizeof(std::size_t) + sizeof(double) * result.size(), 0);
      std::memcpy(result.data(), &resultSize, sizeof(resultSize));
      std::memcpy(result.data() + sizeof(resultSize), resultWorkload.data(), sizeof(double) * resultSize);
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

    // std::transform_reduce is in C++17, here in C++14
    const auto result = [&]() -> double {
      auto tmp_local = input;
      std::transform(input.cbegin(), input.cend(), tmp_local.begin(),
                     [](const double in) -> double { return in * in * in; });
      return std::accumulate(tmp_local.cbegin(), tmp_local.cend(), 0.0);
    }();
    std::vector<double> output(nbOutputBytes / 8, 0);
    const std::size_t output_size = output.size();
    const auto double_output_size = static_cast<double>(output_size);

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
