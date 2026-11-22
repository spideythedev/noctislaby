#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nle {

using ConnectionID = std::uint64_t;

template <typename... Args>
class Signal final {
public:
    using Callback = std::function<void(Args...)>;

    Signal() = default;

    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    [[nodiscard]] ConnectionID connect(Callback callback) {
        if (!callback) {
            return 0;
        }

        std::lock_guard lock(mutex_);

        if (next_connection_id_ == 0 ||
            next_connection_id_ == std::numeric_limits<ConnectionID>::max()) {
            return 0;
        }

        const ConnectionID id = next_connection_id_++;
        callbacks_.emplace(id, std::move(callback));
        return id;
    }

    [[nodiscard]] bool disconnect(const ConnectionID id) {
        std::lock_guard lock(mutex_);
        return callbacks_.erase(id) != 0;
    }

    [[nodiscard]] std::size_t connection_count() const {
        std::lock_guard lock(mutex_);
        return callbacks_.size();
    }

    void emit(Args... args) const {
        std::vector<Callback> callbacks;

        {
            std::lock_guard lock(mutex_);
            callbacks.reserve(callbacks_.size());

            for (const auto& [id, callback] : callbacks_) {
                static_cast<void>(id);
                callbacks.push_back(callback);
            }
        }

        for (const Callback& callback : callbacks) {
            callback(args...);
        }
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<ConnectionID, Callback> callbacks_;
    ConnectionID next_connection_id_ = 1;
};

} // namespace nle
