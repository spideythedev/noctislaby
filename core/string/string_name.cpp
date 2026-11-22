#include "string_name.h"

#include <functional>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace nle {
namespace {

class StringNamePool final {
public:
    std::shared_ptr<const std::string> intern(const std::string_view value) {
        if (value.empty()) {
            return {};
        }

        std::lock_guard lock(mutex_);

        std::string key(value);
        const auto found = entries_.find(key);
        if (found != entries_.end()) {
            if (auto existing = found->second.lock()) {
                return existing;
            }
        }

        auto interned = std::make_shared<const std::string>(std::move(key));
        entries_[*interned] = interned;

        ++insertions_since_cleanup_;
        if (insertions_since_cleanup_ >= cleanup_interval_) {
            cleanup();
        }

        return interned;
    }

private:
    void cleanup() {
        for (auto entry = entries_.begin(); entry != entries_.end();) {
            if (entry->second.expired()) {
                entry = entries_.erase(entry);
            } else {
                ++entry;
            }
        }

        insertions_since_cleanup_ = 0;
    }

    static constexpr std::size_t cleanup_interval_ = 256;

    std::mutex mutex_;
    std::unordered_map<std::string, std::weak_ptr<const std::string>> entries_;
    std::size_t insertions_since_cleanup_ = 0;
};

StringNamePool& string_name_pool() {
    static StringNamePool pool;
    return pool;
}

} // namespace

StringName::StringName(const std::string_view value)
    : value_(string_name_pool().intern(value)) {}

std::string_view StringName::view() const noexcept {
    return value_ == nullptr ? std::string_view{} : std::string_view(*value_);
}

std::string StringName::str() const {
    return std::string(view());
}

bool StringName::empty() const noexcept {
    return view().empty();
}

std::size_t StringName::hash() const noexcept {
    return std::hash<std::string_view>{}(view());
}

bool operator==(const StringName& left, const StringName& right) noexcept {
    if (left.value_ == right.value_) {
        return true;
    }

    return left.view() == right.view();
}

std::strong_ordering operator<=>(
    const StringName& left,
    const StringName& right
) noexcept {
    return left.view() <=> right.view();
}

} // namespace nle
