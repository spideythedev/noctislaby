#pragma once

#include <mutex>
#include <string_view>

namespace nle {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger final {
public:
    static Logger& instance();

    void write(LogLevel level, std::string_view message);

    void debug(std::string_view message);
    void info(std::string_view message);
    void warning(std::string_view message);
    void error(std::string_view message);

private:
    Logger() = default;

    std::mutex mutex_;
};

} // namespace nle
