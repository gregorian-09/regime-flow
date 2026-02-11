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

struct Error {
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

    Error()
        : code(Code::Unknown),
          message(),
          details(std::nullopt),
          location(std::source_location::current()) {}

    Error(Code c, std::string msg,
          std::source_location loc = std::source_location::current())
        : code(c), message(std::move(msg)), location(loc) {}

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

    std::string to_string() const {
        std::ostringstream out;
        out << "[" << static_cast<int>(code) << "] " << message
            << " (at " << location.file_name() << ":" << location.line() << ")";
        return out.str();
    }
};

template<typename T>
class Result {
public:
    Result(T value) : value_(std::move(value)) {}
    Result(Error error) : value_(std::move(error)) {}

    bool is_ok() const { return std::holds_alternative<T>(value_); }
    bool is_err() const { return std::holds_alternative<Error>(value_); }

    T& value() & {
        if (is_err()) throw std::runtime_error(error().to_string());
        return std::get<T>(value_);
    }
    const T& value() const& {
        if (is_err()) throw std::runtime_error(error().to_string());
        return std::get<T>(value_);
    }
    T&& value() && {
        if (is_err()) throw std::runtime_error(error().to_string());
        return std::move(std::get<T>(value_));
    }

    Error& error() & { return std::get<Error>(value_); }
    const Error& error() const& { return std::get<Error>(value_); }

    template<typename F>
    auto map(F&& f) -> Result<std::invoke_result_t<F, T>> {
        if (is_ok()) {
            return Result<std::invoke_result_t<F, T>>(f(value()));
        }
        return error();
    }

    template<typename F>
    auto and_then(F&& f) -> std::invoke_result_t<F, T> {
        if (is_ok()) {
            return f(value());
        }
        return error();
    }

    T value_or(T default_value) const {
        return is_ok() ? value() : default_value;
    }

private:
    std::variant<T, Error> value_;
};

template<>
class Result<void> {
public:
    Result() = default;
    Result(Error error) : error_(std::move(error)) {}

    bool is_ok() const { return !error_.has_value(); }
    bool is_err() const { return error_.has_value(); }

    const Error& error() const { return *error_; }

private:
    std::optional<Error> error_;
};

template<typename T>
Result<T> Ok(T value) { return Result<T>(std::move(value)); }

inline Result<void> Ok() { return Result<void>(); }

#if defined(__cpp_lib_format)

template<typename... Args>
Error Err(Error::Code code, std::string_view fmt, Args&&... args) {
    return Error(code, std::vformat(fmt, std::make_format_args(args...)));
}

#else

template<typename... Args>
Error Err(Error::Code code, std::string_view fmt, Args&&...) {
    return Error(code, std::string(fmt));
}

#endif

#define TRY(expr) \
    ({ \
        auto _result = (expr); \
        if (_result.is_err()) return _result.error(); \
        std::move(_result).value(); \
    })

}  // namespace regimeflow
