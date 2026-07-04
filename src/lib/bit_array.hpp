/*
 * bit_array.hpp
 * Copyright (c) 2013 Yasuo Tabei All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE and * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef SMBT_BIT_ARRAY_HPP_
#define SMBT_BIT_ARRAY_HPP_

#include <iostream>
#include <vector>
#include <stdint.h>

namespace smbt {

class BitArray {
private:
public:
  BitArray() {
    tail_ = 0;
  }
  void     set_bit(uint64_t bit, uint64_t pos);
  uint64_t get_bit(uint64_t pos) const;
  uint64_t get_val(uint64_t start, uint64_t end) const;
  void     push(uint64_t bit);
  void     save(std::ostream &os);
  void     load(std::istream &is);
  uint64_t size_in_bytes() const {
    uint64_t size = (64 * bitarray_.size())/8;
    return size;
  }
  uint64_t get_size() const {
    return bitarray_.size() * kBlockSize;
  }
private:
  static uint64_t kBlockSize;
  static uint64_t kClip[64][64];
  std::vector<uint64_t> bitarray_;
  uint64_t tail_;
};

}  // namespace smbt

#endif // SMBT_BIT_ARRAY_HPP_
