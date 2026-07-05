/*
 * error.hpp
 *
 * smbt::Error — the exception type used throughout the library for
 * recoverable runtime failures (unreadable input, empty database,
 * corrupt/truncated index). The CLI drivers (build.cpp/search.cpp) and
 * the Python binding both catch it at their top level and turn it into a
 * clean error message + non-zero exit / Python exception, instead of the
 * old exit(1) that would kill an embedding interpreter.
 */

#ifndef SMBT_ERROR_HPP_
#define SMBT_ERROR_HPP_

#include <stdexcept>
#include <string>

namespace smbt {

class Error : public std::runtime_error {
 public:
  explicit Error(const std::string &msg) : std::runtime_error(msg) {}
};

}  // namespace smbt

#endif  // SMBT_ERROR_HPP_
