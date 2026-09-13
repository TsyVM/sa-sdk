// SPDX-License-Identifier: MIT
//
// src/error.cpp
// =============
// Human-readable strings for sasdk::ErrorKind.  Lives in the static lib so
// the string table is not duplicated across every TU that includes result.hpp.
#include <sasdk/core/result.hpp>

namespace sasdk {

std::string_view error_kind_name(ErrorKind k) noexcept {
    switch (k) {
    case ErrorKind::NotFound:   return "not_found";
    case ErrorKind::OutOfRange: return "out_of_range";
    case ErrorKind::Unverified: return "unverified";
    case ErrorKind::IoError:    return "io_error";
    case ErrorKind::BadFormat:  return "bad_format";
    default:                    return "unknown";
    }
}

} // namespace sasdk
