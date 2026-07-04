/*
 * checked_read.hpp
 *
 * Shared `checked_read` helper: reads `n` bytes from `is` into `buf` and
 * exits with a clear error message if the stream is short or corrupt.
 * This single definition replaces five near-identical copies (two
 * file-static in src/lib, three inline in the mbt/vla/trie module
 * headers).
 */

#ifndef SMBT_CHECKED_READ_HPP_
#define SMBT_CHECKED_READ_HPP_

#include <cstdlib>
#include <iostream>

namespace smbt {

inline void checked_read(std::istream &is, char *buf, std::streamsize n) {
  is.read(buf, n);
  if (!is) {
    std::cerr << "error: corrupt or truncated index file" << std::endl;
    exit(1);
  }
}

}  // namespace smbt

#endif // SMBT_CHECKED_READ_HPP_
