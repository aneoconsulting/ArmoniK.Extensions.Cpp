#pragma once

namespace ArmoniK {
namespace Sdk {
namespace Client {
namespace Internal {

/**
 * @brief A helper class to batch requests
 *
 * @tparam Request The request type
 */
template <class Request> class Batcher {
private:
  /**
   * @brief The batch size
   */
  std::size_t batch_size_;
  /**
   * @brief The current requests
   */
  std::vector<Request> requests_;
  /**
   * @brief The function to process a batch
   */
  std::function<void(std::vector<Request> &&)> f_;

public:
  /**
   * @brief Construct a new Batcher object
   *
   * @param batch_size The batch size
   * @param f The function to process a batch
   */
  explicit Batcher(int batch_size, std::function<void(std::vector<Request> &&)> f)
      : batch_size_(batch_size), f_(std::move(f)) {
    requests_.reserve(batch_size_);
  }

  /**
   * @brief Copy constructor
   */
  Batcher(const Batcher &) = delete;
  /**
   * @brief Copy assignment
   */
  Batcher &operator=(const Batcher &) = delete;

  /**
   * @brief Move constructor
   */
  Batcher(Batcher &&other) noexcept = default;
  /**
   * @brief Move assignment
   */
  Batcher &operator=(Batcher &&other) noexcept = default;

  /**
   * @brief Destroy the Batcher object
   *
   * @details
   * The destructor does NOT process any pending requests.
   * Any remaining requests must be explicitly flushed by calling ProcessBatch().
   *
   * This avoids throwing exceptions from the destructor
   */
  ~Batcher() noexcept = default;

  /**
   * @brief Add a request to the batcher, processing the batch if full
   *
   * @param request The request to add
   */
  void Add(Request request) {
    requests_.push_back(static_cast<Request &&>(request));
    if (requests_.size() >= batch_size_) {
      ProcessBatch();
    }
  }

  /**
   * @brief Process all the pending requests
   */
  void ProcessBatch() {
    if (requests_.empty()) {
      return;
    }

    f_(std::move(requests_));
    requests_.clear();
    requests_.reserve(batch_size_);
  }
};

} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
