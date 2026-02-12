#ifndef ADAS_CORE_LOGGER_HPP
#define ADAS_CORE_LOGGER_HPP

/**
 * @file Logger.hpp
 * @brief Severity-tagged logger inspired by AUTOSAR DEM (Diagnostic Event Manager).
 *
 * Provides per-component named logging with severity levels.
 * Thread-safe via simple mutex; suitable for development and diagnostics.
 */

#include <iostream>
#include <string>
#include <sstream>
#include <mutex>
#include <chrono>
#include <iomanip>

namespace adas {

/**
 * @brief Log severity levels aligned with AUTOSAR DEM event severity.
 */
enum class LogLevel : int {
    DEBUG   = 0,   ///< Development-only diagnostic messages
    INFO    = 1,   ///< Normal operational messages
    WARN    = 2,   ///< Potential issue detected
    ERROR   = 3,   ///< Recoverable error
    FATAL   = 4    ///< Unrecoverable error, system should halt
};

/**
 * @brief Lightweight logger with component tagging.
 *
 * Usage:
 * @code
 *   adas::Logger log("PlannerSWC");
 *   log.info("Trajectory computed with cost: {}", cost);
 * @endcode
 */
class Logger {
public:
    explicit Logger(std::string component_name)
        : component_name_(std::move(component_name)) {}

    /// Set minimum severity threshold for output
    static void setGlobalLevel(LogLevel level) { global_level_ = level; }
    static LogLevel globalLevel() { return global_level_; }

    void debug(const std::string& msg) const { log(LogLevel::DEBUG, msg); }
    void info(const std::string& msg)  const { log(LogLevel::INFO,  msg); }
    void warn(const std::string& msg)  const { log(LogLevel::WARN,  msg); }
    void error(const std::string& msg) const { log(LogLevel::ERROR, msg); }
    void fatal(const std::string& msg) const { log(LogLevel::FATAL, msg); }

private:
    std::string component_name_;
    inline static LogLevel global_level_ = LogLevel::INFO;
    inline static std::mutex mutex_;

    void log(LogLevel level, const std::string& msg) const {
        if (level < global_level_) return;

        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::lock_guard<std::mutex> lock(mutex_);
        std::cerr << "[" << std::put_time(std::localtime(&time_t_now), "%H:%M:%S")
                  << "." << std::setw(3) << std::setfill('0') << ms.count()
                  << "] ["  << levelToString(level)
                  << "] [" << component_name_
                  << "] "  << msg << "\n";
    }

    static const char* levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return " INFO";
            case LogLevel::WARN:  return " WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default:              return "?????";
        }
    }
};

} // namespace adas

#endif // ADAS_CORE_LOGGER_HPP
