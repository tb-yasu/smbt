/*
 * checked_read.hpp
 *
 * Shared `checked_read` helper: reads `n` bytes from `is` into `buf` and
 * throws smbt::Error with a clear message if the stream is short or
 * corrupt. This single definition replaces five near-identical copies
 * (two file-static in src/lib, three inline in the mbt/vla/trie module
 * headers). Throwing (rather than exit(1)) lets the CLI and the Python
 * binding report the failure cleanly instead of aborting the process.
 */

#ifndef SMBT_CHECKED_READ_HPP_
#define SMBT_CHECKED_READ_HPP_

#include <istream>

#include "error.hpp"

namespace smbt {

inline void checked_read(std::istream &is, char *buf, std::streamsize n) {
  is.read(buf, n);
  if (!is) {
    throw Error("error: corrupt or truncated index file");
  }
}

}  // namespace smbt

#endif // SMBT_CHECKED_READ_HPP_
