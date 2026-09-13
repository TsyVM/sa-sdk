// include/sasdk/core/result.hpp
//
// Phase 4 scaffold: the error vocabulary every other SASDK header builds on.
// No exceptions, no global error state.
//
// NOTE: std::expected is C++23, not C++20 -- confirmed by compile failure
// while building this scaffold (gcc: "'expected' is only available from
// C++23 onwards"). MWSDK's README states a C++20 core target, so pulling in
// std::expected here would either silently break that promise or force
// every consumer to C++23. Ship a small in-house Result<T> instead so the
// C++20 claim is actually true; revisit only if the project later decides
// C++23 is an acceptable baseline for everyone, not just an opt-in adapter.
#pragma once
#include <string>
#include <cstdint>
#include <variant>
#include <optional>
#include <utility>

namespace sasdk {

enum class ErrorKind : uint8_t {
    NotFound,       // e.g. a schema key / field / symbol that doesn't exist
    OutOfRange,     // read/write outside a known valid extent
    Unverified,     // the data exists but is tier=open/unverified and the
                     // caller asked for a verified-only guarantee
    IoError,        // file/process read failure
    BadFormat,       // file didn't parse as expected
};

struct Error {
    ErrorKind kind;
    std::string message;
};

template <typename T>
class Result {
public:
    Result(T value) : storage_(std::move(value)) {}
    Result(Error error) : storage_(std::move(error)) {}

    explicit operator bool() const { return std::holds_alternative<T>(storage_); }
    bool has_value() const { return std::holds_alternative<T>(storage_); }

    T&       operator*()       { return std::get<T>(storage_); }
    const T& operator*() const { return std::get<T>(storage_); }
    T*       operator->()       { return &std::get<T>(storage_); }
    const T* operator->() const { return &std::get<T>(storage_); }

    const Error& error() const { return std::get<Error>(storage_); }

    T value_or(T fallback) const {
        return has_value() ? std::get<T>(storage_) : std::move(fallback);
    }

private:
    std::variant<T, Error> storage_;
};

// void specialization: "did this succeed", no payload.
class Status {
public:
    Status() : error_(std::nullopt) {}
    Status(Error error) : error_(std::move(error)) {}

    explicit operator bool() const { return !error_.has_value(); }
    const Error& error() const { return *error_; }

private:
    std::optional<Error> error_;
};

inline Error err(ErrorKind kind, std::string message) {
    return Error{kind, std::move(message)};
}

// Defined in src/error.cpp (SASDK::data static lib).
// Returns a short snake_case label for the error kind.
std::string_view error_kind_name(ErrorKind k) noexcept;

} // namespace sasdk

