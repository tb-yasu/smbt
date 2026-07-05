"""smbt — Succinct Multibit Tree.

Fast Jaccard/Tanimoto similarity search over chemical fingerprints, backed by
succinct data structures (Tabei, WABI 2012). Each fingerprint is a set of
uint32 item ids.

Quick start
-----------
>>> import smbt
>>> data = [[1, 5, 9], [2, 5, 8], [1, 5, 9, 12]]
>>> idx = smbt.build(data, mode=1, minsup=10)
>>> idx.search([1, 5, 9], similarity=0.5)   # -> [(0, 1.0), (2, 0.75), ...]
>>> idx.save("idx.smbt")
>>> smbt.load("idx.smbt").search([1, 5, 9], similarity=0.9)

`mode` selects the representation (all three return identical results):
1 = succinct multibit tree + trie (smallest), 2 = + variable-length array,
3 = pointer-based multibit tree (baseline). Result ids are the 0-based
position of each fingerprint in the input.
"""

from ._core import Index, Error

try:  # populated from package metadata once installed
    from importlib.metadata import version as _pkg_version

    __version__ = _pkg_version("smbt")
except Exception:  # pragma: no cover - source tree / editable without metadata
    __version__ = "0+unknown"


def build(fingerprints, mode=1, minsup=10, verbose=False):
    """Build an index from an iterable of fingerprints (each a list of ints)."""
    return Index(list(fingerprints), mode=mode, minsup=minsup, verbose=verbose)


def build_file(path, mode=1, minsup=10, verbose=False):
    """Build an index from a fingerprint file (one fingerprint per line)."""
    return Index.from_file(str(path), mode=mode, minsup=minsup, verbose=verbose)


def load(path):
    """Load an index previously written with Index.save()."""
    return Index.load(str(path))


__all__ = ["Index", "Error", "build", "build_file", "load", "__version__"]
