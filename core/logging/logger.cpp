#include "logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

namespace nle {
namespace {

const char* level_name(const LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error: return "ERROR";
    }

    return "UNKNOWN";
}

} // namespace

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::write(const LogLevel level, const std::string_view message) {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;

    std::lock_guard lock(mutex_);

    std::tm local_time{};
#if defined(_WIN32)
    localtime_s(&local_time, &time);
#else
    localtime_r(&time, &local_time);
#endif

    std::ostream& output = level == LogLevel::Error ? std::cerr : std::clog;
    output << '[' << std::put_time(&local_time, "%H:%M:%S")
           << '.' << std::setfill('0') << std::setw(3) << milliseconds.count()
           << "] [" << level_name(level) << "] " << message << '\n';
}

void Logger::debug(const std::string_view message) {
    write(LogLevel::Debug, message);
}

void Logger::info(const std::string_view message) {
    write(LogLevel::Info, message);
}

void Logger::warning(const std::string_view message) {
    write(LogLevel::Warning, message);
}

void Logger::error(const std::string_view message) {
    write(LogLevel::Error, message);
}

} // namespace nle
