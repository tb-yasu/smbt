/*
 * suc_array.cpp
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

#include "suc_array.hpp"
#include "checked_read.hpp"

using namespace std;

namespace smbt {

uint64_t SucArray::get_len(uint64_t val) {
  size_t p;
  for (p = 63; p > 0; --p) {
    if ((val >> p) & 1ULL)
      break;
  }
  return p + 1;
}

void SucArray::build() {
  pointersb_.Build(pointers_);
  // move-assign a fresh builder (not Clear(): vector::clear() keeps capacity)
  pointersb_ = rsdic::RSDicBuilder();
}

void SucArray::push(uint64_t val) {
  ++size_;
  uint64_t len = get_len(val);
  for (size_t i = 0; i < len; ++i) {
    if ((val >> i) & 1ULL)
      bits_.push(1);
    else
      bits_.push(0);
  }
  for (size_t i = 1; i < len; ++i) {
    pointersb_.PushBack(0);
  }
  pointersb_.PushBack(1);
}

uint64_t SucArray::get(uint64_t pos) const {
  uint64_t end = pointers_.Select(pos, 1);
  uint64_t start;
  if (pos != 0)
    start = pointers_.Select(pos - 1, 1) + 1;
  else
    start = 0;
  /*
  uint64_t val = 0;
  for (uint64_t i = start; i <= end; ++i) {
    if (bits_.get_bit(i))
      val |= (1ULL << (i - start));
  }
  */
  /*
  if (val != bits_.bits_.get_val(start, end)) {
  bits_.get_val(start, end);
  }
  */
  return bits_.get_val(start, end);
}

void SucArray::save(ostream &os) {
  bits_.save(os);
  pointers_.Save(os);
  os.write((const char*)(&size_), sizeof(size_));
}

void SucArray::load(istream &is) {
  bits_.load(is);
  pointers_.Load(is);
  /*
  if (bits_.get_size() == 0 && pointers_.num() != 0) {
    cerr << bits_.get_size() << " " << pointers_.num() << endl;
    exit(1);
  }
  */
  checked_read(is, (char*)(&size_), sizeof(size_));
}

}  // namespace smbt
