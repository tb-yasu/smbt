/*
 * smbt_core.cpp — pybind11 bindings for the Succinct Multibit Tree.
 * Copyright (c) 2013-2026 Yasuo Tabei. MIT License (see LICENSE).
 *
 * Exposes a single Python type, smbt._core.Index, that wraps the three C++
 * implementations (mode 1 = trie, 2 = vla, 3 = mbt) behind one interface.
 * The 8-byte format tag (mode + 10) that the CLI writes/reads outside the
 * classes is handled here in save()/load(), mirroring build.cpp/search.cpp.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <algorithm>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "lib/error.hpp"
#include "smbt_trie/succinct_multibit_tree_trie.hpp"
#include "smbt_vla/succinct_multibit_tree_vla.hpp"
#include "mbt/multibit_tree.hpp"

namespace py = pybind11;

namespace {

typedef std::vector<std::vector<uint32_t> > FpData;
typedef std::vector<std::pair<uint64_t, double> > Hits;  // (db id, similarity)

void validate_mode(int mode) {
  if (mode != 1 && mode != 2 && mode != 3)
    throw std::invalid_argument("mode must be 1, 2 or 3");
}

void validate_similarity(double similarity) {
  if (similarity < 0.0 || similarity > 1.0)
    throw std::invalid_argument("similarity must be in [0, 1]");
}

// Coerce any path-like (str, os.PathLike, pathlib.Path) to a std::string.
// Must be called with the GIL held (it invokes os.fspath).
std::string to_path(const py::object &pathobj) {
  py::object s = py::module_::import("os").attr("fspath")(pathobj);
  return s.cast<std::string>();
}

// Sort hits into a deterministic order: similarity descending, ties by id
// ascending. Jaccard here is a ratio of integer counts, so it is bit-stable
// across the three modes and the resulting order is identical for all.
void sort_hits(Hits &hits) {
  struct Cmp {
    bool operator()(const std::pair<uint64_t, double> &a,
                    const std::pair<uint64_t, double> &b) const {
      if (a.second != b.second) return a.second > b.second;
      return a.first < b.first;
    }
  };
  std::sort(hits.begin(), hits.end(), Cmp());
}

class Index {
 public:
  // --- factories -----------------------------------------------------------
  static Index *build(const FpData &data, int mode, uint64_t minsup, bool verbose) {
    validate_mode(mode);
    std::unique_ptr<Index> self(new Index(mode));
    if (mode == 1) { self->trie_->set_verbose(verbose); self->trie_->build_from_data(data, minsup); }
    else if (mode == 2) { self->vla_->set_verbose(verbose); self->vla_->build_from_data(data, minsup); }
    else { self->mbt_->set_verbose(verbose); self->mbt_->build_from_data(data, minsup); }
    return self.release();
  }

  static Index *build_file(const py::object &pathobj, int mode, uint64_t minsup, bool verbose) {
    validate_mode(mode);
    const std::string path = to_path(pathobj);
    py::gil_scoped_release nogil;
    std::unique_ptr<Index> self(new Index(mode));
    if (mode == 1) { self->trie_->set_verbose(verbose); self->trie_->build(path.c_str(), minsup); }
    else if (mode == 2) { self->vla_->set_verbose(verbose); self->vla_->build(path.c_str(), minsup); }
    else { self->mbt_->set_verbose(verbose); self->mbt_->build(path.c_str(), minsup); }
    return self.release();
  }

  static Index *load(const py::object &pathobj) {
    const std::string path = to_path(pathobj);
    py::gil_scoped_release nogil;
    std::ifstream ifs(path.c_str(), std::ios::in | std::ios::binary);
    if (!ifs)
      throw smbt::Error("cannot open: " + path);
    uint64_t tag = 0;
    ifs.read((char *)&tag, sizeof(tag));
    if (!ifs)
      throw smbt::Error("error: corrupt or truncated index file: " + path);

    int mode;
    if (tag == 11) mode = 1;
    else if (tag == 12) mode = 2;
    else if (tag == 13) mode = 3;
    else if (tag == 1 || tag == 2 || tag == 3)
      throw smbt::Error("error: " + path +
                        " is an old index format (v1) — rebuild the index with smbt-build.");
    else {
      std::ostringstream oss;
      oss << "error: " << path << " is not a smbt index file (unknown format tag " << tag << ").";
      throw smbt::Error(oss.str());
    }

    std::unique_ptr<Index> self(new Index(mode));
    if (mode == 1) self->trie_->load(ifs);
    else if (mode == 2) self->vla_->load(ifs);
    else self->mbt_->load(ifs);
    return self.release();
  }

  // --- persistence ---------------------------------------------------------
  void save(const py::object &pathobj) const {
    const std::string path = to_path(pathobj);
    py::gil_scoped_release nogil;
    std::ofstream ofs(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!ofs)
      throw smbt::Error("cannot open: " + path);
    const uint64_t tag = (uint64_t)mode_ + 10;
    ofs.write((const char *)&tag, sizeof(tag));
    if (mode_ == 1) trie_->save(ofs);
    else if (mode_ == 2) vla_->save(ofs);
    else mbt_->save(ofs);
    ofs.flush();
    if (!ofs)
      throw smbt::Error("error writing index file: " + path);
  }

  // --- query ---------------------------------------------------------------
  // Not const: the underlying search_fv/search_query are not marked const,
  // but they only read the succinct structures, so releasing the GIL and
  // running searches concurrently is data-race-free.
  Hits search(const std::vector<uint32_t> &query, double similarity) {
    validate_similarity(similarity);
    Hits hits;
    if (mode_ == 1) {
      std::vector<std::pair<float, uint32_t> > res;
      trie_->search_fv(query, (float)similarity, res);
      for (size_t i = 0; i < res.size(); ++i)
        hits.push_back(std::make_pair((uint64_t)res[i].second, (double)res[i].first));
    } else if (mode_ == 2) {
      std::vector<std::pair<float, uint32_t> > res;
      vla_->search_fv(query, (float)similarity, res);
      for (size_t i = 0; i < res.size(); ++i)
        hits.push_back(std::make_pair((uint64_t)res[i].second, (double)res[i].first));
    } else {
      std::vector<std::pair<float, uint64_t> > res;
      mbt_->search_fv(query, (float)similarity, res);
      for (size_t i = 0; i < res.size(); ++i)
        hits.push_back(std::make_pair(res[i].second, (double)res[i].first));
    }
    sort_hits(hits);
    return hits;
  }

  int mode() const { return mode_; }

  uint64_t size_in_bytes() {
    if (mode_ == 1) return trie_->size_in_bytes();
    if (mode_ == 2) return vla_->size_in_bytes();
    return mbt_->size_in_bytes();
  }

 private:
  explicit Index(int mode) : mode_(mode) {
    if (mode_ == 1) trie_.reset(new smbt::trie::SuccinctMultibitTreeTRIE());
    else if (mode_ == 2) vla_.reset(new smbt::vla::SuccinctMultibitTreeVLA());
    else mbt_.reset(new smbt::mbt::MultibitTree());
  }

  int mode_;
  std::unique_ptr<smbt::trie::SuccinctMultibitTreeTRIE> trie_;
  std::unique_ptr<smbt::vla::SuccinctMultibitTreeVLA>   vla_;
  std::unique_ptr<smbt::mbt::MultibitTree>              mbt_;
};

}  // namespace

PYBIND11_MODULE(_core, m) {
  m.doc() = "Succinct Multibit Tree: Jaccard/Tanimoto similarity search over fingerprints.";

  py::register_exception<smbt::Error>(m, "Error", PyExc_RuntimeError);

  py::class_<Index>(m, "Index",
                    "A built (or loaded) similarity-search index. Build with "
                    "Index(fingerprints, mode=..., minsup=...), Index.from_file(path, ...), "
                    "or Index.load(path). Not thread-safe for build/save/load, but "
                    "search() releases the GIL and is safe to call concurrently.")
      .def(py::init(&Index::build),
           py::arg("fingerprints"),
           py::arg("mode") = 1,
           py::arg("minsup") = 10,
           py::arg("verbose") = false,
           py::call_guard<py::gil_scoped_release>(),
           "Build an index from a list of fingerprints (each a list of uint32 "
           "item ids). An empty fingerprint raises ValueError.")
      .def_static("from_file", &Index::build_file,
                  py::arg("path"),
                  py::arg("mode") = 1,
                  py::arg("minsup") = 10,
                  py::arg("verbose") = false,
                  "Build an index from a fingerprint file (one whitespace-separated "
                  "line per fingerprint; blank lines are skipped).")
      .def_static("load", &Index::load,
                  py::arg("path"),
                  "Load an index previously written by save(). Raises smbt.Error for "
                  "a v1 index (rebuild required) or an unrecognized file.")
      .def("save", &Index::save,
           py::arg("path"),
           "Write the index to a file, tagged with its mode.")
      .def("search", &Index::search,
           py::arg("query"),
           py::arg("similarity") = 0.9,
           py::call_guard<py::gil_scoped_release>(),
           "Return [(db_id, similarity), ...] for every database fingerprint whose "
           "Jaccard/Tanimoto similarity to the query is >= similarity, sorted by "
           "similarity descending then id ascending. db_id is the 0-based input "
           "index of the fingerprint. An empty query raises ValueError.")
      .def_property_readonly("mode", &Index::mode, "The index mode (1, 2 or 3).")
      .def("size_in_bytes", &Index::size_in_bytes,
           "Approximate size of the succinct multibit tree(s), in bytes.")
      .def("__repr__", [](Index &self) {
        std::ostringstream oss;
        oss << "<smbt.Index mode=" << self.mode() << ">";
        return oss.str();
      });
}
