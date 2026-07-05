/*
 * multibit_tree.hpp
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

#ifndef SMBT_MULTIBIT_TREE_HPP_
#define SMBT_MULTIBIT_TREE_HPP_

#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <bitset>
#include <cmath>
#include <cstring>
#include <string>
#include <iterator>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <stack>
#include <stdint.h>
#include <time.h>
#include <utility>

#include "../lib/checked_read.hpp"

namespace smbt {
namespace mbt {

class CardinalityLess {
public:
  bool operator()(const std::pair<uint64_t, std::vector<uint32_t> > *x, const std::pair<uint64_t, std::vector<uint32_t> > *y) const {
    return (x->second.size() < y->second.size());
  }
};

struct Node {
  bool                  leaf;
  std::vector<uint32_t> one_col, zero_col;
  std::vector<uint32_t> ids;

  Node() {
    one_col.clear(); zero_col.clear(); ids.clear();
    leaf = false;
  }

  uint64_t size_in_bytes() {
    uint64_t sum = 1; // for leaf
    sum += (one_col.size() + zero_col.size() + ids.size()) * 4;
    return sum;
  }

  void save(std::ostream &os) {
    os.write((const char*)&leaf, sizeof(bool));
    {
      size_t size = one_col.size();
      os.write((const char*)&size, sizeof(size_t));
      os.write((const char*)&one_col[0], sizeof(uint32_t)*size);
    }
    {
      size_t size = zero_col.size();
      os.write((const char*)&size, sizeof(size_t));
      os.write((const char*)&zero_col[0], sizeof(uint32_t)*size);
    }
    {
      size_t size = ids.size();
      os.write((const char*)&size, sizeof(size_t));
      os.write((const char*)&ids[0], sizeof(uint32_t)*size);
    }
  }
  void read(std::istream &is) {
    checked_read(is, (char*)&leaf, sizeof(bool));
    {
      size_t size;
      checked_read(is, (char*)&size, sizeof(size_t));
      one_col.resize(size);
      checked_read(is, (char*)&one_col[0], sizeof(uint32_t)*size);
    }
    {
      size_t size;
      checked_read(is, (char*)&size, sizeof(size_t));
      zero_col.resize(size);
      checked_read(is, (char*)&zero_col[0], sizeof(uint32_t)*size);
    }
    {
      size_t size;
      checked_read(is, (char*)&size, sizeof(size_t));
      ids.resize(size);
      checked_read(is, (char*)&ids[0], sizeof(uint32_t)*size);
    }
  }
};

struct Tree {
  uint32_t              cardinality;
  std::vector<Node>     nodes;
  std::vector<uint32_t> left_child;
  std::vector<uint32_t> right_child;
  Tree() {
    cardinality = 0; nodes.clear(); left_child.clear(); right_child.clear();
  }

  uint64_t size_in_bytes() {
    uint64_t sum = 4; // for cardinality
    for (size_t i = 0; i < nodes.size(); ++i) {
      sum += nodes[i].size_in_bytes();
    }

    sum += (left_child.size() + right_child.size()) * 4;
    return sum;
  }

  void save(std::ostream &os) {
    os.write((const char*)&cardinality, sizeof(uint32_t));
    {
      size_t size = nodes.size();
      os.write((const char*)&size, sizeof(size_t));
      for (size_t i = 0; i < size; ++i)
	nodes[i].save(os);
    }
    {
      size_t size = left_child.size();
      os.write((const char*)&size, sizeof(size_t));
      os.write((const char*)&left_child[0], sizeof(uint32_t)*size);
    }
    {
      size_t size = right_child.size();
      os.write((const char*)&size, sizeof(size_t));
      os.write((const char*)&right_child[0], sizeof(uint32_t)*size);
    }
  }
  void load(std::istream &is) {
    checked_read(is, (char*)&cardinality, sizeof(uint32_t));
    {
      size_t size;
      checked_read(is, (char*)&size, sizeof(size_t));
      nodes.resize(size);
      for (size_t i = 0; i < size; ++i)
	nodes[i].read(is);
    }
    {
      size_t size;
      checked_read(is, (char*)&size, sizeof(size_t));
      left_child.resize(size);
      checked_read(is, (char*)&left_child[0], sizeof(uint32_t)*size);
    }
    {
      size_t size;
      checked_read(is, (char*)&size, sizeof(size_t));
      right_child.resize(size);
      checked_read(is, (char*)&right_child[0], sizeof(uint32_t)*size);
    }
  }
};

class MultibitTree {
private:
  // fvs_ holds raw `new`ed pairs, so the class owns heap memory and must
  // not be copied (a shallow copy would double-free). Declared, undefined.
  MultibitTree(const MultibitTree &);
  MultibitTree &operator=(const MultibitTree &);
  void  free_fvs();
  void  read_file(std::ifstream &ifs, std::vector<std::pair<uint64_t, std::vector<uint32_t> >* > &fvs);
  float calc_entropy(uint64_t count_one, uint64_t count_zero);
  void  build_range_multibit_tree(uint64_t num_one, uint64_t start, uint64_t end);
  void  build_multibit_tree(const std::vector<std::pair<uint64_t, std::vector<uint32_t> >* > &fvs);
  void  set_column_info(std::set<uint64_t> &dims, const std::vector<uint32_t> &ids, Tree &tree, uint32_t cur, const std::vector<uint32_t> &counter);
  void  build_recursive(Tree &tree, uint32_t cur, std::set<uint64_t> dims, std::vector<uint32_t> &ids);
  void  search_query(const std::vector<uint32_t> &qfv, float similarity, std::vector<std::pair<float, uint64_t> > &res);
  void  search_query_recursive(Tree &tree, uint64_t cur, const std::vector<uint32_t> &qfv, float similarity, uint32_t one_col_num, uint32_t zero_col_num, std::vector<std::pair<float, uint64_t> > &res);
  void  calc_column_info(Tree &tree, uint64_t cur, const std::vector<uint32_t> &qfv, uint32_t &one_col_num, uint32_t &zero_col_num);
  bool  pruning(Tree &tree, const std::vector<uint32_t> &qfv, float similarity, uint32_t one_col_num, uint32_t zero_col_num);
  void  calc_similarity(Tree &tree, uint64_t cur, const std::vector<uint32_t> &qfv, float similarity, std::vector<std::pair<float, uint64_t> > &res);
  float calc_jaccard_sim(const std::vector<uint32_t> &ba1, const std::vector<uint32_t> &ba2);
public:
  MultibitTree() : minsup_(0), dim_(0) {}
  ~MultibitTree();
  void  print_memory();
  void  build(const char *fname, uint64_t minsup);
  void  search(const char *qname, float similarity);
  void  save(std::ostream &os);
  void  load(std::istream &is);

  uint64_t size_in_bytes() {
    uint64_t sum = 0;
    for (size_t i = 0; i < fvs_.size(); ++i)  {
      sum += 4;
      sum += fvs_[i]->second.size() * 4;
    }
    return sum;
  }
private:
  uint64_t                                                   minsup_;
  std::vector<std::pair<uint64_t, std::vector<uint32_t> >* > fvs_;
  std::vector<Tree>                                          trees_;
  uint64_t                                                   dim_;
};

}  // namespace mbt
}  // namespace smbt
#endif // SMBT_MULTIBIT_TREE_HPP_
