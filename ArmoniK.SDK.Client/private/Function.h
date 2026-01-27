#pragma once

#include <armonik/common/logger/formatter.h>
#include <armonik/common/logger/logger.h>
#include <armonik/common/logger/writer.h>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace ArmoniK {
namespace Sdk {
namespace Client {
namespace Internal {

/**
 * @brief A type-erased function wrapper
 *
 * @tparam Signature The function signature
 */
template <class> class Function;

/**
 * @brief A type-erased function wrapper
 *
 * @tparam Ret The return type
 * @tparam Args The argument types
 */
template <class Ret, class... Args> class Function<Ret(Args...)> {
private:
  /**
   * @brief Type-erased callable interface
   */
  struct Callable {
    /**
     * @brief Virtual destructor
     */
    virtual ~Callable() = default;

    /**
     * @brief Call the function
     */
    virtual Ret call(Args... args) = 0;
  };

  /**
   * @brief Template implementation of the callable interface
   */
  template <class F> struct Impl : public Callable {
    F f_;
    explicit Impl(F f) : f_(std::move(f)) {}
    Ret call(Args... args) override { return f_(static_cast<Args &&>(args)...); }
  };

private:
  /**
   * @brief The type-erased callable
   */
  std::unique_ptr<Callable> callable_{};

public:
  /**
   * @brief Default constructor
   */
  Function() noexcept = default;

  /**
   * @brief Constructs a Function from a callable
   */
  template <class F> Function(F f) : callable_(std::make_unique<Impl<F>>(std::move(f))) {}

  /**
   * @brief Copy constructor (deleted)
   */
  Function(const Function &other) = delete;
  /**
   * @brief Copy assignment operator (deleted)
   */
  Function &operator=(const Function &other) = delete;

  /**
   * @brief Move constructor
   */
  Function(Function &&other) noexcept = default;
  /**
   * @brief Move assignment operator
   */
  Function &operator=(Function &&other) noexcept = default;

  /**
   * @brief Destructor
   */
  ~Function() = default;

  /**
   * @brief Call the function
   */
  Ret operator()(Args... args) { return callable_->call(static_cast<Args &&>(args)...); }
};

} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
