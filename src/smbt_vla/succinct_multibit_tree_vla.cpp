/*
 * succinct_multibit_tree_vla.cpp
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

#include "succinct_multibit_tree_vla.hpp"

using namespace std;

namespace smbt {
namespace vla {
void SuccinctMultibitTreeVLA::read_file(ifstream &ifs, vector<pair<uint32_t, vector<uint32_t> >* > &fvs) {
  string   line;
  uint32_t item;
  uint32_t id = 0;
  while (getline(ifs, line)) {
    vector<uint32_t> fv;
    stringstream ss(line);
    while (ss >> item)
      fv.push_back(item);
    if (fv.empty())
      continue;
    sort(fv.begin(), fv.end());
    fv.erase(unique(fv.begin(), fv.end()), fv.end());
    fvs.resize(fvs.size() + 1);
    fvs[fvs.size() - 1] = new pair<uint32_t, vector<uint32_t> >;
    fvs[fvs.size() - 1]->first = id++;
    fvs[fvs.size() - 1]->second.swap(fv);
  }
}

void SuccinctMultibitTreeVLA::build_range_multibit_tree(uint64_t num_one, uint64_t start, uint64_t end) {
  vector<uint32_t> ids;
  for (uint64_t id = start; id <= end; ++id)
    ids.push_back(id);

  set<uint32_t> items;
  for (size_t i = 0; i < ids.size(); ++i) {
    vector<uint32_t> &fv = fvs_[ids[i]]->second;
    for (size_t j = 0; j < fv.size(); ++j)
      items.insert(fv[j]);
  }

  vector<Component*> components;
  build_multibit_tree_recursive(0, ids, items, components);
  stable_sort(components.begin(), components.end(), DepthLess());
  trees_.resize(trees_.size() + 1);
  trees_[trees_.size() - 1].cardinality = num_one;
  trees_[trees_.size() - 1].build(components);

  for (size_t i = 0; i < components.size(); ++i)
    delete components[i];
}


void SuccinctMultibitTreeVLA::build_multibit_tree() {
  sort(fvs_.begin(), fvs_.end(), CardinalityLess());

  uint64_t start = 0;
  uint64_t bef_num = fvs_[0]->second.size();

  for (size_t i = 1; i < fvs_.size(); ++i) {
    uint64_t cur_num = fvs_[i]->second.size();
    if (bef_num != cur_num) {
      build_range_multibit_tree(bef_num, start, i - 1);
      start = i;
      bef_num = cur_num;
    }
  }
  build_range_multibit_tree(bef_num, start, fvs_.size() - 1);
}

float SuccinctMultibitTreeVLA::calc_entropy(uint32_t num_one, uint32_t num_zero) {
  uint32_t num_all = num_one + num_zero;
  float ent = 0.f;
  if (num_one != 0)
    ent += -((float(num_one)/float(num_all)) * log(float(num_one)/float(num_all)));
  if (num_zero != 0)
    ent += -((float(num_zero)/float(num_all)) * log(float(num_zero)/float(num_all)));

  return ent / log(2.0f);
}

void SuccinctMultibitTreeVLA::calc_split_item(vector<uint32_t> &ids, set<uint32_t> &items, uint32_t &item, const vector<uint32_t> &count_one) {
  float max_ent = 0;
  item = 0;
  for (set<uint32_t>::iterator it = items.begin(); it != items.end(); ++it) {
    uint32_t num_one, num_zero;
    if (count_one.size() <= *it) {
      num_one = 0; num_zero = ids.size();
    }
    else {
      num_one = count_one[*it]; num_zero = ids.size() - num_one;
    }
    float current_ent = calc_entropy(num_one, num_zero);
    if (max_ent < current_ent) {
      max_ent = current_ent;
      item = *it;
    }
    if (max_ent >= 0.95)
      break;
  }

  if (max_ent < 0.001)
    item = UINT_MAX;
}

void SuccinctMultibitTreeVLA::calc_dominant_items(set<uint32_t> &items, vector<uint32_t> &ids, vector<uint32_t> &one_cols, vector<uint32_t> &zero_cols, vector<uint32_t> &delete_items, const vector<uint32_t> &count_one) {
  for (set<uint32_t>::iterator it = items.begin(); it != items.end(); ++it) {
    uint32_t citem = *it;
    if      (count_one.size() <= citem) {
      zero_cols.push_back(citem);
      delete_items.push_back(citem);
    }
    else if (count_one[citem] == ids.size()) {
      one_cols.push_back(citem);
      delete_items.push_back(citem);
    }
    else if (count_one[citem] == 0) {
      zero_cols.push_back(citem);
      delete_items.push_back(citem);
    }
  }
}

void SuccinctMultibitTreeVLA::split_data(uint32_t item, vector<uint32_t> &ids, vector<uint32_t> &one_ids, vector<uint32_t> &zero_ids) {
  for (size_t i = 0; i < ids.size(); ++i)  {
    vector<uint32_t> &fv = fvs_[ids[i]]->second;
    if (binary_search(fv.begin(), fv.end(), item))
      one_ids.push_back(ids[i]);
    else
      zero_ids.push_back(ids[i]);
  }
}

void SuccinctMultibitTreeVLA::build_multibit_tree_recursive(uint32_t depth, vector<uint32_t> &ids, set<uint32_t> items, vector<Component*> &components) {
  components.resize(components.size() + 1);
  components[components.size() - 1] = new Component;
  components[components.size() - 1]->depth = depth;
  vector<uint32_t> &cids = components[components.size() - 1]->ids;
  components[components.size() - 1]->leaf = false;

  if (items.size() == 0 || ids.size() <= minsup_) {
    components[components.size() - 1]->leaf = true;
    cids.resize(ids.size());
    for (size_t i = 0; i < ids.size(); ++i)
      cids[i] = ids[i];
    return;
  }

  vector<uint32_t> count_one;
  for (size_t i = 0; i < ids.size(); ++i) {
    vector<uint32_t> &fv = fvs_[ids[i]]->second;
    for (size_t j = 0; j < fv.size(); ++j) {
      if (count_one.size() <= fv[j])
	count_one.resize(fv[j] + 1);
      ++count_one[fv[j]];
    }
  }

  uint32_t item;
  calc_split_item(ids, items, item, count_one);
  items.erase(item);

  if (ids.size() <= minsup_ || item == UINT_MAX) {
    components[components.size() - 1]->leaf = true;
    cids.resize(ids.size());
    for (size_t i = 0; i < ids.size(); ++i)
      cids[i] = ids[i];
    return;
  }

  vector<uint32_t> &one_cols  = components[components.size() - 1]->one_cols;
  vector<uint32_t> &zero_cols = components[components.size() - 1]->zero_cols;
  vector<uint32_t> delete_items;
  calc_dominant_items(items, ids, one_cols, zero_cols, delete_items, count_one);
  for (size_t i = 0; i < delete_items.size(); ++i)
    items.erase(delete_items[i]);
  delete_items.clear();

  vector<uint32_t> one_ids, zero_ids;
  split_data(item, ids, one_ids, zero_ids);

  build_multibit_tree_recursive(depth + 1, one_ids, items, components);
  build_multibit_tree_recursive(depth + 1, zero_ids, items, components);
}

void SuccinctMultibitTreeVLA::save(ostream &os) {
  {
    size_t size = trees_.size();
    os.write((const char*)(&size), sizeof(size_t));
    for (size_t i = 0; i < size; ++i)
      trees_[i].save(os);
  }
  {
    size_t size = itemset_.size();
    os.write((const char*)(&size), sizeof(size));
    for (size_t i = 0; i < size; ++i) {
      uint32_t id = itemset_[i].first;
      os.write((const char*)(&id), sizeof(id));
      itemset_[i].second.save(os);
    }
  }
}

void SuccinctMultibitTreeVLA::load(istream &is) {
  {
    size_t size;
    checked_read(is, (char*)(&size), sizeof(size_t));
    trees_.resize(size);
    for (size_t i = 0; i < size; ++i)
      trees_[i].load(is);
  }
  {
    size_t size;
    checked_read(is, (char*)(&size), sizeof(size));
    itemset_.resize(size);
    for (size_t i = 0; i < size; ++i) {
      uint32_t id;
      checked_read(is, (char*)(&id), sizeof(id));
      itemset_[i].first = id;
      itemset_[i].second.load(is);
    }
  }
}

void SuccinctMultibitTreeVLA::build_trie() {
  itemset_.resize(fvs_.size());
  for (size_t i = 0; i < fvs_.size(); ++i) {
    itemset_[i].first = fvs_[i]->first;
    vector<uint32_t> &fv = fvs_[i]->second;
    smbt::SucArray &items = itemset_[i].second;
    uint32_t bef = 0;
    for (size_t j = 0; j < fv.size(); ++j) {
      items.push(fv[j] - bef);
      bef = fv[j];
    }
    items.build();
  }
}

void SuccinctMultibitTreeVLA::build(const char *fname, size_t minsup) {
  minsup_ = minsup;
  {
    ifstream ifs(fname);
    if (!ifs) {
      throw smbt::Error(std::string("cannot open: ") + fname);
    }
    read_file(ifs, fvs_);
    ifs.close();
  }
  if (fvs_.empty()) {
    throw smbt::Error(std::string("error: no fingerprints read from ") + fname + " (empty or all-blank input)");
  }

  cerr << "building multibit tree" << endl;
  double bstime = clock();
  build_multibit_tree();
  double betime = clock();

  cerr << "building trie"  << endl;
  double tstime = clock();
  build_trie();
  double tetime = clock();

  double btime = (betime - bstime)/CLOCKS_PER_SEC;
  double ttime = (tetime - tstime)/CLOCKS_PER_SEC;
  cout << "building time of multibittree (sec):" << btime << endl;
  cout << "building time of trie (sec):" << ttime << endl;
  cout << "total building time (sec):" << (btime + ttime) << endl;
  cout << "succinct multibit tree size (byte):" << size_in_bytes() << endl;
  cout << "trie size (byte):" << trie_size_in_bytes() << endl;
}

void SuccinctMultibitTreeVLA::calc_column_info(Tree &tree, uint64_t cur, const vector<uint32_t> &qfv, uint32_t &one_col_num, uint32_t &zero_col_num) {
  uint64_t r = tree.internal_rank(cur);

  vector<uint32_t> one_col;
  tree.one_cols[r].decode(one_col);

  for (size_t i = 0; i < one_col.size(); ++i) {
    if (binary_search(qfv.begin(), qfv.end(), one_col[i]) == false)
      ++one_col_num;
  }

  vector<uint32_t> zero_col;
  tree.zero_cols[r].decode(zero_col);
  for (size_t i = 0; i < zero_col.size(); ++i) {
    if (binary_search(qfv.begin(), qfv.end(), zero_col[i]) == true)
      ++zero_col_num;
  }
}

bool SuccinctMultibitTreeVLA::pruning(Tree &tree, const vector<uint32_t> &qfv, float similarity, uint32_t one_col_num, uint32_t zero_col_num) {
  uint64_t card_a = tree.cardinality;
  uint64_t card_b = qfv.size();
  uint64_t common = std::min(card_a - one_col_num, card_b - zero_col_num);
  return (float(common)/float(card_a + card_b - common) < similarity);
}

float SuccinctMultibitTreeVLA::jaccard_sim(uint32_t id, const vector<uint32_t> &qfv) {
  uint64_t common = 0;
  smbt::SucArray &items = itemset_[id].second;
  size_t i = 0, j = 0;
  size_t size1 = items.get_size();
  size_t size2 = qfv.size();
  uint64_t val = items.get(i);
  while (i < size1 && j < size2) {
    if      (val < qfv[j])  {
      if (++i < size1)
	val += items.get(i);
    }
    else if (val > qfv[j]) {
      ++j;
    }
    else {
      if (++i < size1)
	val += items.get(i);
      ++common; ++j;
    }
  }
  return float(common)/float(size1 + size2 - common);
}

void SuccinctMultibitTreeVLA::calc_similarity(Tree &tree, uint64_t cur, const vector<uint32_t> &qfv, float similarity, vector<pair<float, uint32_t> > &res) {
  vector<uint32_t> ids;
  tree.get_ids(cur, ids);
  float sim = 0.f;

  for (size_t id = 0; id < ids.size(); ++id) {
    if ((sim = jaccard_sim(ids[id], qfv)) >= similarity)
      res.push_back(make_pair(sim, itemset_[ids[id]].first));
  }
}

void SuccinctMultibitTreeVLA::search_query_recursive(Tree &tree, uint64_t cur, const vector<uint32_t> &qfv, float similarity, uint32_t one_col_num, uint32_t zero_col_num, vector<pair<float,  uint32_t> > &res) {
  if (tree.leaf(cur)) {
    calc_similarity(tree, cur, qfv, similarity, res);
    return;
  }

  calc_column_info(tree, cur, qfv, one_col_num, zero_col_num);
  if (pruning(tree, qfv, similarity, one_col_num, zero_col_num)) {
    return;
  }

  search_query_recursive(tree, tree.get_first_child(cur), qfv, similarity, one_col_num, zero_col_num, res);
  search_query_recursive(tree, tree.get_first_child(cur+1), qfv, similarity, one_col_num, zero_col_num, res);
}

void SuccinctMultibitTreeVLA::search_query(const vector<uint32_t> &qfv, float similarity, vector<pair<float,  uint32_t> > &res) {
  uint64_t query_one_num = qfv.size();
  for (size_t i = 0; i < trees_.size(); ++i) {
    uint32_t cardinality = trees_[i].cardinality;
    if (similarity * float(cardinality) <= float(query_one_num) && float(query_one_num) <= float(cardinality) / similarity)  {
      search_query_recursive(trees_[i], 2, qfv, similarity, 0, 0, res);
    }
    if (similarity * float(cardinality) > float(query_one_num))
      break;

  }
}

void SuccinctMultibitTreeVLA::search(const char *qname, float sim) {
  cerr << "loading query file:" << qname << endl;
  vector<pair<uint32_t, vector<uint32_t> >* > qfvs;
  {
    ifstream ifs(qname);
    if (!ifs) {
      throw smbt::Error(std::string("cannot open: ") + qname);
    }
    read_file(ifs, qfvs);
    ifs.close();
  }

  uint64_t total_num = 0;
  vector<double> times;
  for (size_t i = 0; i < qfvs.size(); ++i) {
    uint32_t qid          = qfvs[i]->first;
    vector<uint32_t> &qfv = qfvs[i]->second;
    vector<pair<float,  uint32_t> > res;
    double stime = clock();
    search_query(qfv, sim, res);
    double etime = clock();
    times.push_back((etime - stime)/CLOCKS_PER_SEC);
    total_num += res.size();
    cout << "query id:" << qid << " num:" << res.size() << " ";
    for (size_t j = 0; j < res.size(); ++j) {
      cout << res[j].second << ":" << res[j].first << " ";
    }
    cout << endl;
  }
  size_t size = times.size();
  double mean = 0.0;
  if (size > 0) {
    for (size_t i = 0; i < size; ++i)
      mean += times[i];
    mean /= size;
  }

  double var = 0.0;
  if (size > 1) {
    for (size_t i = 0; i < size; ++i)
      var += (times[i] - mean) * (times[i] - mean);
    var /= (size - 1);
  }
  double stddev = sqrt(var);

  cout << "average answers:" << (size > 0 ? double(total_num)/double(size) : 0.0) << endl;
  cout << "mean time (sec):" << mean << endl;
  cout << "std dev:" << stddev << endl;
}
}
}
