/*
 * succinct_multibit_tree_trie.hpp
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

#ifndef SMBT_SUCCINCT_MULTIBIT_TREE_TRIE_HPP_
#define SMBT_SUCCINCT_MULTIBIT_TREE_TRIE_HPP_

#include <iostream>
#include <vector>
#include <string>
#include <stdint.h>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <stack>
#include <cmath>
#include <algorithm>
#include <queue>
#include <limits.h>

#include "../rsdic/lib/RSDicBuilder.hpp"
#include "../rsdic/lib/RSDic.hpp"
#include "../lib/suc_array.hpp"
#include "../lib/var_byte.hpp"
#include "../lib/checked_read.hpp"

namespace smbt {
namespace trie {

class CardinalityLess {
public:
  bool operator()(const std::pair<uint32_t, std::vector<uint32_t> > *x, const std::pair<uint32_t, std::vector<uint32_t> > *y) const {
    return (x->second.size() < y->second.size());
  }
};

struct Component {
  uint32_t depth;
  std::vector<uint32_t> one_cols;
  std::vector<uint32_t> zero_cols;
  std::vector<uint32_t> ids;
  bool leaf;
  Component() {
    depth = 0; one_cols.clear(); zero_cols.clear(); ids.clear(); leaf = false;
  }
};

class DepthLess {
public:
  bool operator()(const Component *x, const Component *y) const {
    return (x->depth < y->depth);
  }
};

class LexLess {
public:
  bool operator()(const std::pair<uint32_t, std::vector<uint32_t> > *x, const std::pair<uint32_t, std::vector<uint32_t> > *y) {
    const std::vector<uint32_t> &fvx = x->second;
    const std::vector<uint32_t> &fvy = y->second;
    size_t size = std::min(fvx.size(), fvy.size());
    for (size_t i = 0; i < size; ++i) {
      if      (fvx[i] < fvy[i])
	return true;
      else if (fvx[i] > fvy[i])
	return false;
    }
    if (fvx.size() < fvy.size())
      return true;
    else
      return false;
  }
};

struct Tree {
  uint32_t             cardinality;
  rsdic::RSDic         ba;
  rsdic::RSDic         leaves;
  std::vector<smbt::VarByte> one_cols;
  std::vector<smbt::VarByte> zero_cols;
  std::vector<smbt::VarByte> ids;

  Tree() {
    cardinality = 0;
  }

  uint64_t size_in_bytes() const {
    uint64_t sum = 0;
    sum += 4;

    sum += ba.GetUsageBytes();
    sum += leaves.GetUsageBytes();
    for (size_t i = 0; i < one_cols.size(); ++i)
      sum += one_cols[i].size();
    for (size_t i = 0; i < zero_cols.size(); ++i)
      sum += zero_cols[i].size();
    for (size_t i = 0; i < ids.size(); ++i) {
      sum += ids[i].size();
    }
    return sum;
  }

  uint64_t get_first_child(uint64_t cur) const {
    return ba.Select(ba.Rank(cur+1,1)-1, 0) + 1;
  }

  bool leaf(uint64_t cur) const {
    return (bool)leaves.GetBit(cur);
  }

  void get_ids(uint64_t cur, std::vector<uint32_t> &vec) const {
    ids[leaves.Rank(cur, 1)].decode(vec);
  }

  // Rank of the internal node starting at position cur among all internal
  // nodes, in position order. ba is "10" (super-root: one 1-bit) followed
  // by one block per node: "110" for an internal node (two 1-bits), "0"
  // for a leaf (none). Rank(pos,b) counts [0,pos), so the 1-bits before
  // cur are 1 (super-root) + 2 * (number of preceding internal nodes).
  uint64_t internal_rank(uint64_t cur) const {
    return (ba.Rank(cur, 1) - 1) / 2;
  }

  void build(std::vector<Component*> &components) {
    rsdic::RSDicBuilder badb;
    rsdic::RSDicBuilder leavesdb;
    badb.PushBack(1);
    badb.PushBack(0);
    leavesdb.PushBack(0);
    leavesdb.PushBack(0);
    for (size_t i = 0; i < components.size(); ++i) {
      std::vector<uint32_t> &tmp_one_cols  = components[i]->one_cols;
      std::vector<uint32_t> &tmp_zero_cols = components[i]->zero_cols;
      std::vector<uint32_t> &tmp_ids       = components[i]->ids;
      bool                   leaf          = components[i]->leaf;

      if (!leaf) {
	// One entry per internal node, appended in loop (= position) order:
	// entry k belongs to the internal node with internal_rank() == k.
	one_cols.push_back(smbt::VarByte());
	one_cols.back().encode(tmp_one_cols);
	zero_cols.push_back(smbt::VarByte());
	zero_cols.back().encode(tmp_zero_cols);
	leavesdb.PushBack(0);
	leavesdb.PushBack(0);
	leavesdb.PushBack(0);
	badb.PushBack(1);
	badb.PushBack(1);
	badb.PushBack(0);
      }
      else {
	leavesdb.PushBack(1);
	badb.PushBack(0);
	ids.resize(ids.size() + 1);
	ids[ids.size() - 1].encode(tmp_ids);
      }
    }
    badb.Build(ba);
    leavesdb.Build(leaves);
  }

  void save(std::ostream &os) {
    ba.Save(os);
    leaves.Save(os);
    os.write((const char*)(&cardinality), sizeof(cardinality));
    {
      size_t size = one_cols.size();
      os.write((const char*)(&size), sizeof(size));
      for (size_t i = 0; i < size; ++i)
	one_cols[i].save(os);
    }
    {
      size_t size = zero_cols.size();
      os.write((const char*)(&size), sizeof(size));
      for (size_t i = 0; i < size; ++i)
	zero_cols[i].save(os);
    }
    {
      size_t size = ids.size();
      os.write((const char*)(&size), sizeof(size));
      for (size_t i = 0; i < size; ++i)
	ids[i].save(os);
    }
  }

  void load(std::istream &is) {
    ba.Load(is);
    leaves.Load(is);
    checked_read(is, (char*)(&cardinality), sizeof(cardinality));
    {
      size_t size;
      checked_read(is, (char*)(&size), sizeof(size));
      one_cols.resize(size);
      for (size_t i = 0; i < size; ++i) {
	if (one_cols[i].load(is) != 0) {
	  throw smbt::Error("error: corrupt or truncated index file");
	}
      }
    }
    {
      size_t size;
      checked_read(is, (char*)(&size), sizeof(size));
      zero_cols.resize(size);
      for (size_t i = 0; i < size; ++i) {
	if (zero_cols[i].load(is) != 0) {
	  throw smbt::Error("error: corrupt or truncated index file");
	}
      }
    }
    {
      size_t size;
      checked_read(is, (char*)(&size), sizeof(size));
      ids.resize(size);
      for (size_t i = 0; i < size; ++i) {
	if (ids[i].load(is) != 0) {
	  throw smbt::Error("error: corrupt or truncated index file");
	}
      }
    }
  }
};

struct QueueElem {
  uint32_t left;
  uint32_t right;
  uint32_t depth;
  QueueElem(uint32_t left, uint32_t right, uint32_t depth) : left(left), right(right), depth(depth) {}
  void set(uint32_t left, uint32_t right, uint32_t depth) {
    this->left = left; this->right = right; this->depth = depth;
  }
};

class SuccinctMultibitTreeTRIE {
private:
  // fvs_ holds raw `new`ed pairs, so the class owns heap memory and must
  // not be copied (a shallow copy would double-free). Declared, undefined.
  SuccinctMultibitTreeTRIE(const SuccinctMultibitTreeTRIE &);
  SuccinctMultibitTreeTRIE &operator=(const SuccinctMultibitTreeTRIE &);
  void     free_fvs();
  void     read_file(std::ifstream &ifs, std::vector<std::pair<uint32_t, std::vector<uint32_t> >* > &fvs);
  void     build_multibit_tree();
  void     build_multibit_tree_recursive(uint32_t depth, std::vector<uint32_t> &ids, std::set<uint32_t> items, std::vector<Component*> &components);
  void     build_range_multibit_tree(uint64_t num_one, uint64_t start, uint64_t end);
  void     build_trie();
  void     calc_split_item(std::vector<uint32_t> &ids, std::set<uint32_t> &items, uint32_t &item, const std::vector<uint32_t> &count_one);
  float    calc_entropy(uint32_t num_one, uint32_t num_zero);
  void     calc_dominant_items(std::set<uint32_t> &items, std::vector<uint32_t> &ids, std::vector<uint32_t> &one_cols, std::vector<uint32_t> &zero_cols, std::vector<uint32_t> &delete_items, const std::vector<uint32_t> &count_one);
  void     split_data(uint32_t item, std::vector<uint32_t> &ids, std::vector<uint32_t> &one_ids, std::vector<uint32_t> &zero_ids);
  void     search_query(const std::vector<uint32_t> &qfv, float similarity, std::vector<std::pair<float,  uint32_t> > &res);
  void     search_query_recursive(Tree &tree, uint64_t cur, const std::vector<uint32_t> &qfv, float similarity, uint32_t one_col_num, uint32_t zero_col_num, std::vector<std::pair<float,  uint32_t> > &res);
  void     calc_column_info(Tree &tree, uint64_t cur, const std::vector<uint32_t> &qfv, uint32_t &one_col_num, uint32_t &zero_col_num);
  bool     pruning(Tree &tree, const std::vector<uint32_t> &qfv, float similarity, uint32_t one_col_num, uint32_t zero_col_num);
  void     calc_similarity(Tree &tree, uint64_t cur, const std::vector<uint32_t> &qfv, float similarity, std::vector<std::pair<float, uint32_t> > &res);
  float    jaccard_sim(uint32_t id, const std::vector<uint32_t> &qfv);
  uint64_t trie_parent(uint64_t pos);
  bool     trie_leaf(uint64_t pos);
  uint64_t trie_first_child(uint64_t pos);
  uint32_t trie_item(uint64_t pos);
  void     compress_items(std::vector<uint32_t> &titems);
  void     unique();
public:
  SuccinctMultibitTreeTRIE() : minsup_(0) {}
  ~SuccinctMultibitTreeTRIE();
  void build(const char *fname, size_t minsup);
  void search(const char *qname, float sim);
  void save(std::ostream &os);
  void load(std::istream &is);

  uint64_t size_in_bytes() {
    uint64_t sum = 0;
    for (size_t i = 0; i < trees_.size(); ++i)
      sum += trees_[i].size_in_bytes();
    return sum;
  }
  uint64_t trie_size_in_bytes() {
    uint64_t sum = 0;
    sum += vb_loud_.GetUsageBytes();
    sum += items_.size_in_bytes();
    sum += idconverter_.size_in_bytes();
    for (size_t i = 0; i < remains_.size(); ++i)
      sum += remains_[i].size_in_bytes();
    return sum;
  }

private:
  std::vector<std::pair<uint32_t, std::vector<uint32_t> >* > fvs_;
  size_t minsup_;
  std::vector<Tree> trees_;

  rsdic::RSDic                vb_loud_;
  smbt::SucArray               items_;
  smbt::SucArray               idconverter_;
  std::vector<smbt::SucArray>  remains_;
  std::vector<std::vector<uint32_t> > clusters_;
};

}  // namespace trie
}  // namespace smbt
#endif // SMBT_SUCCINCT_MULTIBIT_TREE_TRIE_HPP_
