#pragma once
#include "resource_cache.h"
#include "resource_format_loader.h"
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>
namespace nle {
class ResourceLoadRequest final {
public:
    enum class Status { Invalid, Pending, Loading, Loaded, Failed };
    ResourceLoadRequest() = default;
    [[nodiscard]] bool is_valid() const noexcept;
    [[nodiscard]] Status get_status() const;
    [[nodiscard]] ErrorCode get_error() const;
    [[nodiscard]] Ref<Resource> get_resource() const;
    [[nodiscard]] Ref<Resource> wait(ErrorCode* error = nullptr) const;
private:
    struct State final {
        mutable std::mutex mutex;
        std::condition_variable condition;
        Status status = Status::Pending;
        ErrorCode error = ErrorCode::Failed;
        Ref<Resource> resource;
    };
    explicit ResourceLoadRequest(std::shared_ptr<State> state);
    std::shared_ptr<State> state_;
    friend class ResourceLoader;
};
class ResourceLoader final {
public:
    ResourceLoader() = default;
    ~ResourceLoader();
    ResourceLoader(const ResourceLoader&) = delete;
    ResourceLoader& operator=(const ResourceLoader&) = delete;
    [[nodiscard]] bool add_format_loader(std::shared_ptr<ResourceFormatLoader> loader);
    [[nodiscard]] Ref<Resource> load(std::string_view path, ErrorCode* error = nullptr);
    [[nodiscard]] ResourceLoadRequest load_async(std::string_view path);
    [[nodiscard]] ResourceCache& get_cache() noexcept;
    [[nodiscard]] const ResourceCache& get_cache() const noexcept;
private:
    struct LoadState final {
        std::condition_variable condition;
        bool completed = false;
        Ref<Resource> resource;
        ErrorCode error = ErrorCode::Failed;
    };
    struct AsyncTask final {
        std::string path;
        std::shared_ptr<ResourceLoadRequest::State> state;
    };
    [[nodiscard]] std::shared_ptr<ResourceFormatLoader> find_loader(std::string_view extension) const;
    void worker_loop();
    mutable std::mutex loaders_mutex_;
    std::vector<std::shared_ptr<ResourceFormatLoader>> loaders_;
    ResourceCache cache_;
    std::mutex loads_mutex_;
    std::unordered_map<std::string, std::shared_ptr<LoadState>> in_flight_;
    std::mutex worker_mutex_;
    std::condition_variable worker_condition_;
    std::deque<AsyncTask> async_tasks_;
    std::vector<std::thread> workers_;
    bool stopping_workers_ = false;
};
} // namespace nle
