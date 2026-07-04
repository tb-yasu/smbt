/*
 * var_byte.cpp
 * Copyright (c) 2009 Daisuke Okanohara All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <cassert>
#include "var_byte.hpp"

using namespace std;

namespace smbt {

VarByte::VarByte() {}
VarByte::VarByte(const vector<uint32_t>& v) {
  encode(v);
}
VarByte::~VarByte() {}

void VarByte::encode(const vector<uint32_t>& v){
  uint32_t prev = 0;
  for (size_t i = 0; i < v.size(); ++i){
    uint32_t dif = v[i] - prev;
    while (dif >= 0x80){
      b_.push_back(dif & 0x7F);
      dif >>= 7;
    }
    prev = v[i] + 1;
    b_.push_back(dif + 0x80);
  }
}

void VarByte::decode(vector<uint32_t>& v) const{
  uint32_t output = 0;
  uint32_t prev = 0;
  for (size_t i = 0; i < b_.size(); ++i){
    uint32_t count = 0;
    uint32_t x = 0;
    while (b_[i] < 0x80){
      x += (uint32_t)b_[i] << (7*count);
      ++i;
      ++count;
    }
    x += (uint32_t)(b_[i] - 0x80) << (7*count);
    //    assert(output < v.size());
    v.push_back(x + prev);
    //    v[output] = x + prev;
    prev = v[output] + 1;
    output++;
  }
}

size_t VarByte::size() const{
  return b_.size() * sizeof(b_[0]);
}

int VarByte::save(ostream& ofs) const{
  uint32_t size = static_cast<uint32_t>(b_.size());
  if (!ofs.write((char*)(&size), sizeof(size))) return -1;
  if (size == 0) return 0;
  if (!ofs.write((char*)(&b_[0]), sizeof(b_[0]) * b_.size())) return -1;
  return 0;
}

int VarByte::load(istream& ifs){
  uint32_t size = 0;
  if (!ifs.read((char*)(&size), sizeof(size))) return -1;
  if (size == 0) return 0;
  b_.resize(size);
  if (!ifs.read((char*)(&b_[0]), sizeof(b_[0]) * b_.size())) return -1;
  return 0;
}

}  // namespace smbt
