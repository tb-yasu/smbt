# smbt — Succinct Multibit Tree

Fast Jaccard/Tanimoto similarity search over chemical fingerprints, backed by
succinct data structures. `smbt` indexes large fingerprint databases in little
memory and, given a query fingerprint, returns every database entry whose
Jaccard (Tanimoto) similarity is at or above a threshold.

It is a Python binding around the original C++ implementation from
[Tabei, WABI 2012](https://doi.org/10.1007/978-3-642-33122-0_16) (see
[Citation](#citation)).

## Installation

```sh
pip install smbt
```

Wheels are provided for macOS (x86_64/arm64), Linux (x86_64/aarch64), and
Windows (x86_64) on CPython 3.9-3.13. Other platforms build from the source
distribution and need a C++14 compiler.

## Quick start

```python
import smbt

# Each fingerprint is a set of uint32 item ids (e.g. "on" bits of a
# structural fingerprint). Duplicate ids within a fingerprint are ignored.
data = [
    [1, 5, 9],
    [2, 5, 8],
    [1, 5, 9, 12],
]

idx = smbt.build(data, mode=1, minsup=10)

# Every database fingerprint at similarity >= 0.5 to the query,
# as (id, similarity) sorted by similarity descending then id ascending.
idx.search([1, 5, 9], similarity=0.5)
# -> [(0, 1.0), (2, 0.75)]

idx.save("db.smbt")
smbt.load("db.smbt").search([1, 5, 9], similarity=0.9)
# -> [(0, 1.0)]
```

`id` is the 0-based position of the fingerprint in the input. `similarity` is
the exact Jaccard/Tanimoto coefficient.

Build straight from a file (one whitespace-separated fingerprint per line):

```python
idx = smbt.build_file("fingerprints.dat", mode=1, minsup=10)
```

## API

| Call | Description |
|------|-------------|
| `smbt.build(data, mode=1, minsup=10, verbose=False)` | Build from a list of fingerprints (each a list of ints). |
| `smbt.build_file(path, mode=1, minsup=10, verbose=False)` | Build from a fingerprint file. |
| `smbt.load(path)` | Load an index written by `Index.save`. |
| `Index.search(query, similarity=0.9)` | `[(id, similarity), ...]` above the threshold. |
| `Index.save(path)` | Persist the index. |
| `Index.mode` / `Index.size_in_bytes()` | Mode (1/2/3) and approximate index size. |

`mode` selects the representation; **all three return identical results**:

- **1** — succinct multibit tree + succinct trie (smallest index)
- **2** — succinct multibit tree + variable-length array
- **3** — pointer-based multibit tree (uncompressed baseline)

`minsup` is the minimum number of fingerprints per leaf during construction; it
affects index size and speed, not the result set.

`search()` releases the GIL and is safe to call concurrently from multiple
threads on the same index. `build`, `save`, and `load` are not thread-safe.

Errors map to Python exceptions: invalid `mode`/`similarity`, an empty dataset,
an empty fingerprint, or an empty query raise `ValueError`; an unreadable,
truncated, or unrecognized index raises `smbt.Error`.

## Command-line tools

The original CLI is also available:

```sh
smbt-build  -mode 1 -minsup 10 fingerprints.dat db.smbt
smbt-search -similarity 0.9 db.smbt queries.dat
```

Options must precede the positional arguments. Input format: one fingerprint
per line, whitespace-separated uint32 item ids (treated as a set; blank lines
are skipped).

## Constraints

- **64-bit only.** Wheels are built for 64-bit platforms (x86_64, arm64); 32-bit
  is not supported (the format and the algorithm assume a 64-bit `size_t`).
- **Index files are not portable across architectures.** They are a raw binary
  dump assuming 64-bit, same-endianness, same `size_t` width. Build and read an
  index on the same kind of machine (x86_64 and arm64 are both little-endian, so
  this is rarely an issue in practice).
- **Item ids are treated as a dense dimension.** Very large ids (e.g. 2^31)
  inflate memory; map your fingerprint bits to a compact id range first.
- Indexes written by pre-0.1 builds (format v1) are not readable; rebuild them.
- In-memory `build` treats an empty fingerprint as an error, whereas the file
  path skips blank lines. A blank-line-free file and the equivalent in-memory
  data produce a byte-identical index.

## How it works

Fingerprints are grouped by cardinality; each group is indexed by one binary
tree built by recursively splitting on the item that maximizes entropy. Internal
nodes record the columns that are all-ones / all-zeros within their subtree;
accumulating these gives an admissible Jaccard upper bound for branch-and-bound
pruning, and leaves compute the exact Jaccard. Because pruning is admissible and
leaves are exact, the result set is independent of the tree shape — which is why
all three modes agree. Modes 1 and 2 encode the trees and the fingerprint store
with succinct data structures (a vendored rank/select dictionary) to shrink the
index; mode 3 is the uncompressed pointer-based baseline.

## Citation

If you use this software in academic work, please cite:

> Yasuo Tabei. "Succinct Multibit Tree: Compact Representation of Multibit Trees
> by Using Succinct Data Structures in Chemical Fingerprint Searches." In
> *Algorithms in Bioinformatics — 12th International Workshop (WABI 2012)*,
> Lecture Notes in Computer Science, vol. 7534, pp. 201–213. Springer, 2012.
> [doi:10.1007/978-3-642-33122-0_16](https://doi.org/10.1007/978-3-642-33122-0_16)

```bibtex
@inproceedings{tabei2012succinct,
  author    = {Yasuo Tabei},
  title     = {Succinct Multibit Tree: Compact Representation of Multibit Trees
               by Using Succinct Data Structures in Chemical Fingerprint
               Searches},
  booktitle = {Algorithms in Bioinformatics -- 12th International Workshop,
               {WABI} 2012, Ljubljana, Slovenia},
  series    = {Lecture Notes in Computer Science},
  volume    = {7534},
  pages     = {201--213},
  publisher = {Springer},
  year      = {2012},
  doi       = {10.1007/978-3-642-33122-0_16}
}
```

## License

MIT (Yasuo Tabei). Bundles the vendored **rsdic** rank/select library
(BSD-3-Clause, © 2012 Daisuke Okanohara) and a variable-byte coder (MIT, © 2009
Daisuke Okanohara). See [LICENSE](LICENSE) for full third-party notices.
