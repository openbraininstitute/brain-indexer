#pragma once

#include <iostream>
#include <cstdlib>
#include <functional>
#include <boost/format.hpp>

namespace spatial_index {
enum class LogSeverity : unsigned char {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

inline const std::string& to_string(LogSeverity severity) {
    const static std::array<std::string, 5> severities{
        "DEBUG",
        "INFO",
        "WARNING",
        "ERROR",
        "??"
    };

    return severities[std::min(size_t(severity), severities.size() - 1)];
}

/** \brief A logger with supporting basic functionality.
 *
 * This logger delegates the logging task to a callback. This level of
 * indirection enables using the default Python logger from C++; or
 * integrating SI into some custom logging solution.
 *
 * Using this class directly to log is not intended. Rather you should use
 *   - `log_info`
 *   - `log_warn`
 *   - `log_error`
 *
 * or for temporary debugging purposes `LOG_DEBUG` and `LOG_DEBUG_IF`.
 *
 * This is intended to used as a singleton, via `get_global_logger()`.
 */
class Logger {
    public:
        using callback_type = std::function<void(LogSeverity, const std::string&)>;

    public:
        Logger() = delete;
        Logger(const Logger&) = delete;
        Logger(Logger&&) = delete;

        Logger(callback_type cb) : _cb(std::move(cb)) {}

        Logger& operator=(const Logger&) = delete;
        Logger& operator=(Logger&&) = delete;

        inline void log(LogSeverity severity, const std::string& message) {
            _cb(severity, message);
        }

        inline void set_logging_callback(callback_type cb) {
            _cb = std::move(cb);
        }

    private:
        callback_type _cb;
};

/** \brief Safe logger for very early messages.
 *
 * There is a short phase before the default logger has been set up
 * properly where messages may need to be logged. In those case, this
 * function is the standard way in SI to log such messages.
 *
 * Note: this only affects the set up of the default logger; and should be
 * used very sparingly.
 */
void log_fallback(LogSeverity severity, const std::string& message) {
    std::cout << to_string(severity) << ": " << message << std::endl;
}

void default_logging_callback(LogSeverity severity, const std::string& message) {
    std::cout << to_string(severity) << ": " << message << std::endl;
}

/** \brief Obtain a reference to the logger used by SI.
 *
 * This uses a Meyers singleton, to ensure that the global logger is
 * initialized with a safe fallback logger, before it is used.
 *
 * When using a custom logging callback, such as the default logger from
 * Python. There is a phase during start up of the application, where the
 * custom logger hasn't been registered yet; but a message needs to already
 * be logged. This message will be logged with some fallback logger.
 *
 * Note: You probably don't need to call this function explicitly.
 */
inline Logger& get_global_logger() {
    static auto logger = Logger(&default_logging_callback);
    return logger;
}

/// \brief Sets the callback that's used by the logger.
inline void register_logging_callback(Logger::callback_type cb) {
    auto& logger = get_global_logger();
    logger.set_logging_callback(std::move(cb));
}

/// \brief Log a `message` with severity `severity`.
inline void log(LogSeverity severity, const std::string& message) {
    auto& logger = get_global_logger();
    logger.log(severity, message);
}

inline void log(LogSeverity severity, const boost::format& message) {
    log(severity, message.str());
}

/// \brief Log messages that are useful to non-developers.
template<class T>
inline void log_info(const T& message) {
    log(LogSeverity::INFO, message);
}

/// \brief Log issues that users should be aware of.
template<class T>
inline void log_warn(const T& message) {
    log(LogSeverity::WARN, message);
}

/// \brief Log additional information about an error.
template<class T>
inline void log_error(const T& message) {
    log(LogSeverity::ERROR, message);
}

/** \brief Fetches the minimum log severity from the environment.
 *
 * The environment variable `SI_LOG_SEVERITY` can be used to control
 * the minimum severity that log messages need to have. All messages
 * with a lower severity are not logged.
 *
 * The permitted values are, DEBUG, INFO, WARN and ERROR. Note that DEBUG
 * requires that SI was built with `SI_ENABLE_LOG_DEBUG`.
 */
inline LogSeverity get_environment_minimum_log_severity() {
    char const * const var_c_str = std::getenv("SI_LOG_SEVERITY");

    if(var_c_str != nullptr) {
        auto var = std::string(var_c_str);

        if(var == to_string(LogSeverity::DEBUG)) {
            return LogSeverity::DEBUG;
        }
        else if(var == to_string(LogSeverity::INFO)) {
            return LogSeverity::INFO;
        }
        else if(var == to_string(LogSeverity::WARN)) {
            return LogSeverity::WARN;
        }
        else if(var == to_string(LogSeverity::ERROR)) {
            return LogSeverity::ERROR;
        }
        else {
            // Since it is reasonable that the default logger might want
            // to know the minimum log severity, we use the fallback logger.
            log_fallback(
                LogSeverity::WARN,
                "Invalid environment variable 'SI_LOG_SEVERITY'. Defaulting to INFO."
            );
            return LogSeverity::INFO;
        }
    }

    return LogSeverity::INFO;
}

/// \brief A non-const reference to the minimum log severity.
inline LogSeverity& get_mutable_global_minimum_log_severity() {
    static LogSeverity severity = get_environment_minimum_log_severity();
    return severity;
}

/// \brief The minimum log severity that should be printed.
inline const LogSeverity& get_global_minimum_log_severity() {
    return get_mutable_global_minimum_log_severity();
}

/// \brief Set the minimum log severity that should be printed.
inline void set_global_minimum_log_severity(LogSeverity severity) {
    auto& global_severity = get_mutable_global_minimum_log_severity();
    global_severity = severity;
}
}

// In SI debug logging is only temporary, as a form of `prinf` debugging. The
// advantage is that these log messages can be more easily redicted into files
// separated by MPI rank, say.
#define SI_LOG_DEBUG(message) spatial_index::log(spatial_index::LogSeverity::DEBUG, (message));

// Useful, for the common pattern: if ...; then log something.
#define SI_LOG_DEBUG_IF(cond, message) \
    if((cond)) { \
        SI_LOG_DEBUG((message)); \
    }
