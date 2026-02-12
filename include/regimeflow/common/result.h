/**
 * @file result.h
 * @brief RegimeFlow regimeflow result declarations.
 */

#pragma once

#include <optional>
#include <source_location>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#if defined(__cpp_lib_format)
#include <format>
#endif

namespace regimeflow {

/**
 * @brief Structured error information returned by Result.
 */
struct Error {
    /**
     * @brief Error category codes used across the system.
     */
    enum class Code {
        Ok = 0,
        InvalidArgument,
        NotFound,
        AlreadyExists,
        OutOfRange,
        InvalidState,
        IoError,
        ParseError,
        ConfigError,
        PluginError,
        BrokerError,
        NetworkError,
        TimeoutError,
        InternalError,
        Unknown
    };

    Code code = Code::Unknown;
    std::string message;
    std::optional<std::string> details;
    std::source_location location;

    /**
     * @brief Construct an unknown error with current source location.
     */
    Error()
        : code(Code::Unknown),
          message(),
          details(std::nullopt),
          location(std::source_location::current()) {}

    /**
     * @brief Construct an error with explicit code and message.
     * @param c Error code.
     * @param msg Human-readable message.
     * @param loc Source location where error was created.
     */
    Error(Code c, std::string msg,
          std::source_location loc = std::source_location::current())
        : code(c), message(std::move(msg)), location(loc) {}

    /**
     * @brief Copy constructor.
     */
    Error(const Error& other)
        : code(other.code),
          message(other.message),
          details(other.details),
          location(other.location) {}

    Error& operator=(const Error& other) {
        if (this == &other) {
            return *this;
        }
        code = other.code;
        message = other.message;
        details = other.details;
        location = other.location;
        return *this;
    }

    Error(Error&&) noexcept = default;
    Error& operator=(Error&&) noexcept = default;

    /**
     * @brief Render the error as a single-line string.
     * @return Formatted string with code, message, and source location.
     */
    std::string to_string() const {
        std::ostringstream out;
        out << "[" << static_cast<int>(code) << "] " << message
            << " (at " << location.file_name() << ":" << location.line() << ")";
        return out.str();
    }
};

/**
 * @brief Result type that carries either a value or an Error.
 * @tparam T Value type on success.
 */
template<typename T>
class Result {
public:
    /**
     * @brief Construct a success result.
     * @param value Value to store.
     */
    Result(T value) : value_(std::move(value)) {}
    /**
     * @brief Construct an error result.
     * @param error Error to store.
     */
    Result(Error error) : value_(std::move(error)) {}

    /**
     * @brief True if the result holds a value.
     */
    bool is_ok() const { return std::holds_alternative<T>(value_); }
    /**
     * @brief True if the result holds an error.
     */
    bool is_err() const { return std::holds_alternative<Error>(value_); }

    /**
     * @brief Access the value or throw on error.
     * @return Mutable reference to value.
     * @throws std::runtime_error if the result is an error.
     */
    T& value() & {
        if (is_err()) throw std::runtime_error(error().to_string());
        return std::get<T>(value_);
    }
    /**
     * @brief Access the value or throw on error.
     * @return Const reference to value.
     * @throws std::runtime_error if the result is an error.
     */
    const T& value() const& {
        if (is_err()) throw std::runtime_error(error().to_string());
        return std::get<T>(value_);
    }
    /**
     * @brief Move the value out or throw on error.
     * @return Rvalue reference to value.
     * @throws std::runtime_error if the result is an error.
     */
    T&& value() && {
        if (is_err()) throw std::runtime_error(error().to_string());
        return std::move(std::get<T>(value_));
    }

    /**
     * @brief Access the stored error.
     * @return Mutable reference to error.
     */
    Error& error() & { return std::get<Error>(value_); }
    /**
     * @brief Access the stored error.
     * @return Const reference to error.
     */
    const Error& error() const& { return std::get<Error>(value_); }

    /**
     * @brief Map a value to another Result without changing error state.
     * @tparam F Callable type.
     * @param f Mapping function.
     * @return Result of the mapped value, or existing error.
     */
    template<typename F>
    auto map(F&& f) -> Result<std::invoke_result_t<F, T>> {
        if (is_ok()) {
            return Result<std::invoke_result_t<F, T>>(f(value()));
        }
        return error();
    }

    /**
     * @brief Chain a fallible computation.
     * @tparam F Callable that returns Result<U>.
     * @param f Continuation function.
     * @return Result from the continuation, or existing error.
     */
    template<typename F>
    auto and_then(F&& f) -> std::invoke_result_t<F, T> {
        if (is_ok()) {
            return f(value());
        }
        return error();
    }

    /**
     * @brief Return the value or a default if this is an error.
     * @param default_value Value to return on error.
     * @return Stored value or default.
     */
    T value_or(T default_value) const {
        return is_ok() ? value() : default_value;
    }

private:
    std::variant<T, Error> value_;
};

template<>
/**
 * @brief Specialization of Result for void.
 */
class Result<void> {
public:
    /**
     * @brief Construct a successful void result.
     */
    Result() = default;
    /**
     * @brief Construct an error result.
     * @param error Error to store.
     */
    Result(Error error) : error_(std::move(error)) {}

    /**
     * @brief True if the result has no error.
     */
    bool is_ok() const { return !error_.has_value(); }
    /**
     * @brief True if the result carries an error.
     */
    bool is_err() const { return error_.has_value(); }

    /**
     * @brief Access the stored error.
     * @return Const reference to error.
     */
    const Error& error() const { return *error_; }

private:
    std::optional<Error> error_;
};

template<typename T>
/**
 * @brief Helper to construct a successful Result<T>.
 * @tparam T Value type.
 * @param value Value to wrap.
 * @return Result containing the value.
 */
Result<T> Ok(T value) { return Result<T>(std::move(value)); }

/**
 * @brief Helper to construct a successful Result<void>.
 * @return Success result.
 */
inline Result<void> Ok() { return Result<void>(); }

#if defined(__cpp_lib_format)

template<typename... Args>
/**
 * @brief Format an error using std::format if available.
 * @tparam Args Format arguments.
 * @param code Error code.
 * @param fmt Format string.
 * @param args Format arguments.
 * @return Error with formatted message.
 */
Error Err(Error::Code code, std::string_view fmt, Args&&... args) {
    return Error(code, std::vformat(fmt, std::make_format_args(args...)));
}

#else

template<typename... Args>
/**
 * @brief Construct an error with a raw string when std::format is unavailable.
 * @tparam Args Ignored.
 * @param code Error code.
 * @param fmt String to use as the message.
 * @return Error with the provided message.
 */
Error Err(Error::Code code, std::string_view fmt, Args&&...) {
    return Error(code, std::string(fmt));
}

#endif

/**
 * @brief Propagate an error Result early, otherwise unwrap the value.
 * @param expr Expression yielding Result<T>.
 * @return Unwrapped value or early-returned Error.
 */
#define TRY(expr) \
    ({ \
        auto _result = (expr); \
        if (_result.is_err()) return _result.error(); \
        std::move(_result).value(); \
    })

}  // namespace regimeflow
