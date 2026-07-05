/*
 * multibit_tree.cpp
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

#include "multibit_tree.hpp"

using namespace std;

namespace smbt {
namespace mbt {
void MultibitTree::read_file(ifstream &ifs, vector<pair<uint64_t, vector<uint32_t> >* > &fvs) {
  string line;
  uint32_t val = 0;
  uint64_t line_count = 0;
  while (getline(ifs, line)) {
    vector<uint32_t> fv;
    istringstream is(line);
    while (is >> val)
      fv.push_back(val);
    if (fv.empty())
      continue;
    sort(fv.begin(), fv.end());
    fv.erase(unique(fv.begin(), fv.end()), fv.end());
    fvs.resize(fvs.size() + 1);
    fvs[fvs.size() - 1] = new std::pair<uint64_t, std::vector<uint32_t> >();
    fvs[fvs.size() - 1]->first = line_count++;
    fvs[fvs.size() - 1]->second.swap(fv);
  }
}

float MultibitTree::calc_entropy(uint64_t count_one, uint64_t count_zero) {
  uint64_t count_all = count_one + count_zero;
  float ent = 0.f;
  if (count_one != 0)
    ent += -(float(count_one)/float(count_all)) * log(float(count_one)/float(count_all));
  if (count_zero != 0)
    ent += -((float(count_zero)/float(count_all)) * log(float(count_zero)/float(count_all))) ;
  return ent / log(2.0f);
}

void MultibitTree::build_range_multibit_tree(uint64_t num_one, uint64_t start, uint64_t end) {
  vector<uint32_t> ids;
  for (uint32_t id = start; id <= end; ++id)
    ids.push_back(id);

  set<uint64_t> dims;
  for (size_t i = 0; i < ids.size(); ++i) {
    vector<uint32_t> &fv = fvs_[ids[i]]->second;
    for (size_t j = 0; j < fv.size(); ++j)
      dims.insert(fv[j]);
  }

  trees_.resize(trees_.size() + 1);
  Tree &tree = trees_[trees_.size() - 1];
  tree.cardinality = num_one;
  tree.nodes.resize(1);

  std::set<uint32_t> checker;
  build_recursive(tree, 0, dims, ids);
}

void MultibitTree::build_recursive(Tree &tree, uint32_t cur, set<uint64_t> dims, vector<uint32_t> &ids) {
  vector<uint32_t> counter;
  for (size_t i = 0; i < ids.size(); ++i) {
    vector<uint32_t> &fv = fvs_[ids[i]]->second;
    for (size_t j = 0; j < fv.size(); ++j) {
      if (counter.size() <= fv[j])
	counter.resize(fv[j] + 1);
      counter[fv[j]]++;
    }
  }

  set_column_info(dims, ids, tree, cur, counter);

  if (dims.size() == 0 || ids.size() <= minsup_) {
    tree.nodes[cur].ids  = ids;
    tree.nodes[cur].leaf = true;
    return;
  }

  float    max_ent = 0.f;
  uint64_t item = *dims.begin();
  for (set<uint64_t>::iterator it = dims.begin(); it != dims.end(); ++it) {
    uint64_t d         = *it;
    float ent = calc_entropy(counter[d], ids.size() - counter[d]);
    if (ent > max_ent) {
      max_ent = ent;
      item   = d;
    }
    if (max_ent >= 0.95) {
	break;
    }
  }

  vector<uint32_t> child_one_ids, child_zero_ids;
  for (size_t i = 0; i < ids.size(); ++i) {
    if (binary_search(fvs_[ids[i]]->second.begin(), fvs_[ids[i]]->second.end(), item))
      child_one_ids.push_back(ids[i]);
    else
      child_zero_ids.push_back(ids[i]);
  }
  dims.erase(item);

  if (child_zero_ids.size() > 0) {
    tree.nodes.resize(tree.nodes.size() + 1);
    uint32_t child_cur = tree.nodes.size() - 1;
    if (tree.left_child.size() <= cur)
      tree.left_child.resize(cur + 1);
    tree.left_child[cur] = child_cur;
    build_recursive(tree, child_cur, dims, child_zero_ids);
  }

  if (child_one_ids.size() > 0) {
    tree.nodes.resize(tree.nodes.size() + 1);
    uint32_t child_cur = tree.nodes.size() - 1;
    if (tree.right_child.size() <= cur)
      tree.right_child.resize(cur + 1);
    tree.right_child[cur] = child_cur;
    build_recursive(tree, child_cur, dims, child_one_ids);
  }
}

void MultibitTree::build_multibit_tree(const vector<pair<uint64_t,vector<uint32_t> >* > &fvs) {
  uint64_t start = 0;
  uint64_t bef_num = fvs[0]->second.size();
  for (size_t i = 1; i < fvs.size(); ++i) {
    uint64_t cur_num = fvs[i]->second.size();
    if (bef_num != cur_num) {
      build_range_multibit_tree(bef_num, start, i - 1);
      start = i;
      bef_num = cur_num;
    }
  }
  build_range_multibit_tree(bef_num, start, fvs.size() - 1);
}

void MultibitTree::set_column_info(set<uint64_t> &dims, const vector<uint32_t> &ids, Tree &tree, uint32_t cur, const vector<uint32_t> &counter) {
  vector<uint64_t> delete_items;
  for (set<uint64_t>::iterator it = dims.begin(); it != dims.end(); ++it) {
    uint32_t item = *it;
    if (counter.size() <= item) {
      tree.nodes[cur].zero_col.push_back(item);
      delete_items.push_back(item);
    }
    else if (counter[item] == ids.size()) {
      tree.nodes[cur].one_col.push_back(item);
      delete_items.push_back(item);
    }
    else if (counter[item] == 0) {
      tree.nodes[cur].zero_col.push_back(item);
      delete_items.push_back(item);
    }
  }
  for (size_t i = 0; i < delete_items.size(); ++i)
    dims.erase(delete_items[i]);
}

void MultibitTree::build(const char *fname, uint64_t minsup) {
  minsup_ = minsup;
  ifstream ifs(fname);
  if (!ifs) {
    throw smbt::Error(std::string("cannot open: ") + fname);
  }

  cerr << "reading file:" << fname << endl;
  read_file(ifs, fvs_);
  if (fvs_.empty()) {
    throw smbt::Error(std::string("error: no fingerprints read from ") + fname + " (empty or all-blank input)");
  }

  double stime = clock();
  cerr << "sorting" << endl;
  sort(fvs_.begin(), fvs_.end(), CardinalityLess());

  cerr << "build multibit tree" << endl;
  build_multibit_tree(fvs_);
  double etime = clock();
  fprintf(stdout, "construction time (sec):%f\n", (etime - stime)/CLOCKS_PER_SEC);
  print_memory();
  double memory = 0.f;
  for (size_t i = 0; i < fvs_.size(); ++i) {
    memory += 32.f/8.f;
    memory += (fvs_[i]->second.size() * 32)/8.f;
  }
  fprintf(stdout, "input data size (byte):%f\n", memory);
}

void MultibitTree::search_query(const vector<uint32_t> &qfv, float similarity, vector<pair<float, uint64_t> > &res) {
  uint64_t query_one_num = qfv.size();
  for (size_t i = 0; i < trees_.size(); ++i) {
    Tree &tree = trees_[i];

    if (similarity * float(tree.cardinality) <= float(query_one_num) && float(query_one_num) <= float(tree.cardinality) / similarity)
      search_query_recursive(tree, 0, qfv, similarity, 0, 0, res);

    if (similarity * float(tree.cardinality) > float(query_one_num))
      break;
  }
}

void MultibitTree::search_query_recursive(Tree &tree, uint64_t cur, const vector<uint32_t> &qfv, float similarity, uint32_t one_col_num, uint32_t zero_col_num, vector<pair<float, uint64_t> > &res) {
  if (tree.nodes[cur].leaf) {
    calc_similarity(tree, cur, qfv, similarity, res);
    return;
  }
  calc_column_info(tree, cur, qfv, one_col_num, zero_col_num);
  if (pruning(tree, qfv, similarity, one_col_num, zero_col_num)) {
    return;
  }

  search_query_recursive(tree, tree.left_child[cur], qfv, similarity, one_col_num, zero_col_num, res);
  search_query_recursive(tree, tree.right_child[cur], qfv, similarity, one_col_num, zero_col_num, res);
}

bool MultibitTree::pruning(Tree &tree, const vector<uint32_t> &qfv, float similarity, uint32_t one_col_num, uint32_t zero_col_num) {
  uint64_t card_a   = tree.cardinality;
  uint64_t card_b   = qfv.size();
  uint64_t common   = std::min(card_a - one_col_num, card_b - zero_col_num);
  return (float(common)/float(card_a + card_b - common) < similarity);
}

void MultibitTree::calc_similarity(Tree &tree, uint64_t cur, const vector<uint32_t> &qfv, float similarity, vector<pair<float, uint64_t> > &res) {
  vector<uint32_t> &ids = tree.nodes[cur].ids;
  for (size_t id = 0; id < ids.size(); ++id) {
    float sim;
    if ((sim = calc_jaccard_sim(fvs_[ids[id]]->second, qfv)) >= similarity)
      res.push_back(make_pair(sim, fvs_[ids[id]]->first));
  }
}

float MultibitTree::calc_jaccard_sim(const vector<uint32_t> &ba1, const vector<uint32_t> &ba2) {
  uint64_t common = 0;
  size_t i = 0, j = 0;
  while (i < ba1.size() && j < ba2.size()) {
    if      (ba1[i] < ba2[j])   i++;
    else if (ba1[i] > ba2[j])   j++;
    else                        { common++; i++; j++; }
  }
  return float(common)/float(ba1.size() + ba2.size() - common);
}

void MultibitTree::calc_column_info(Tree &tree, uint64_t cur, const vector<uint32_t> &qfv, uint32_t &one_col_num, uint32_t &zero_col_num) {
  const vector<uint32_t> &one_col  = tree.nodes[cur].one_col;
  const vector<uint32_t> &zero_col = tree.nodes[cur].zero_col;

  for (size_t i = 0; i < one_col.size(); ++i) {
    if (binary_search(qfv.begin(), qfv.end(), one_col[i])  == false)
      one_col_num++;
  }
  for (size_t i = 0; i < zero_col.size(); ++i) {
    if (binary_search(qfv.begin(), qfv.end(), zero_col[i]) == true)
      zero_col_num++;
  }
}

void MultibitTree::search(const char *qname, float similarity) {
  ifstream ifs(qname);
  if (!ifs) {
    throw smbt::Error(std::string("cannot open: ") + qname);
  }
  std::vector<std::pair<uint64_t, vector<uint32_t> >* > qfvs;
  read_file(ifs, qfvs);

  vector<double> times;
  uint64_t totalres = 0;
  for (size_t i = 0; i < qfvs.size(); ++i) {
    double stime = clock();
    uint64_t          qid = qfvs[i]->first;
    vector<uint32_t> &qfv = qfvs[i]->second;
    vector<pair<float, uint64_t> > res;
    search_query(qfv, similarity, res);
    totalres += res.size();
    double etime = clock();
    times.push_back((etime - stime)/CLOCKS_PER_SEC);

    cout << "query id:" << qid << " num:" << res.size() << " ";
    for (size_t j = 0; j < res.size(); ++j)
      cout << res[j].second << ":" << res[j].first << " ";
    cout << endl;
  }
  size_t size = times.size();
  double mean = 0.f;
  if (size > 0) {
    for (size_t i = 0; i < size; ++i) {
      mean += times[i];
    }
    mean /= double(size);
  }
  double var = 0.f;
  if (size > 1) {
    for (size_t i = 0; i < size; ++i) {
      var += (times[i] - mean) * (times[i] - mean);
    }
    var /= double(size - 1);
  }
  fprintf(stdout, "mean search time (sec):%f\n", mean);
  fprintf(stdout, "var search time (sec):%f\n", var);
  double sum = (size > 0) ? double(totalres)/double(size) : 0.0;
  fprintf(stdout, "result per query:%f\n", sum);
}

void MultibitTree::print_memory() {
  double memory = 0.f;
  for (size_t i = 0; i < trees_.size(); ++i)
    memory += trees_[i].size_in_bytes();

  fprintf(stdout, "multibit tree size (byte):%f\n", memory);
}

void MultibitTree::save(ostream &os) {
  os.write((const char*)&minsup_, sizeof(uint64_t));
  os.write((const char*)&dim_, sizeof(uint64_t));
  {
    size_t size = fvs_.size();
    os.write((const char*)&size, sizeof(size_t));
    for (size_t i = 0; i < size; ++i) {
      uint64_t         &id = fvs_[i]->first;
      vector<uint32_t> &fv = fvs_[i]->second;
      size_t         fsize = fv.size();
      os.write((const char*)&id, sizeof(uint64_t));
      os.write((const char*)&fsize, sizeof(size_t));
      os.write((const char*)&fv[0], sizeof(uint32_t) * fsize);
    }
  }
  {
    size_t size = trees_.size();
    os.write((const char*)&size, sizeof(size_t));
    for (size_t i = 0; i < size; ++i)
      trees_[i].save(os);
  }
}

void MultibitTree::load(istream &is)  {
  checked_read(is, (char*)&minsup_, sizeof(uint64_t));
  checked_read(is, (char*)&dim_, sizeof(uint64_t));
  {
    size_t size;
    checked_read(is, (char*)&size, sizeof(size_t));
    fvs_.resize(size);
    for (size_t i = 0; i < size; ++i) {
      fvs_[i] = new pair<uint64_t, vector<uint32_t> >;
      uint64_t &id = fvs_[i]->first;
      vector<uint32_t> &bt = fvs_[i]->second;
      size_t fsize;
      checked_read(is, (char*)&id, sizeof(uint64_t));
      checked_read(is, (char*)&fsize, sizeof(size_t));
      bt.resize(fsize);
      checked_read(is, (char*)&bt[0], sizeof(uint32_t) * fsize);
    }
  }
  {
    size_t size;
    checked_read(is, (char*)&size, sizeof(size_t));
    trees_.resize(size);
    for (size_t i = 0; i < size; ++i)
      trees_[i].load(is);
  }
}
}
}
