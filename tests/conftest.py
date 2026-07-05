"""Shared pytest fixtures/helpers for the smbt test suite.

The tests import `smbt` as an installed package (that is how cibuildwheel runs
them against the built wheel). For local runs from the source tree, put the
in-place build on the path first:  PYTHONPATH=python pytest tests/
"""

import random
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parent.parent
DATA_FILE = REPO_ROOT / "dat" / "fingerprints.dat"
GOLDEN_FILE = REPO_ROOT / "test" / "expected_q200_s090.txt"
BUILD_BIN = REPO_ROOT / "prog" / "smbt-build"
SEARCH_BIN = REPO_ROOT / "prog" / "smbt-search"

MODES = (1, 2, 3)


def jaccard(a, b):
    sa, sb = set(a), set(b)
    if not sa and not sb:
        return 1.0
    return len(sa & sb) / float(len(sa | sb))


def brute_force(data, query, similarity):
    """Ground-truth results as a dict {id: sim} for rows at/above threshold."""
    q = set(query)
    out = {}
    for i, fp in enumerate(data):
        s = jaccard(q, fp)
        if s >= similarity:
            out[i] = s
    return out


def make_dataset(n_rows=300, universe=40, seed=0):
    """Deterministic synthetic fingerprints: sets of small item ids, with some
    exact duplicates to exercise the trie unique()/clusters path."""
    rng = random.Random(seed)
    data = []
    for _ in range(n_rows):
        k = rng.randint(1, 8)
        fp = sorted(set(rng.randint(0, universe - 1) for _ in range(k)))
        data.append(fp)
    # force a few exact duplicates
    for j in (5, 17, 42):
        if j < len(data):
            data[j] = list(data[0])
    return data


@pytest.fixture
def dataset():
    return make_dataset()


def requires_data_file():
    if not DATA_FILE.exists():
        pytest.skip(f"sample data {DATA_FILE} not present (not shipped in sdist)")


def requires_golden():
    if not GOLDEN_FILE.exists():
        pytest.skip(f"golden file {GOLDEN_FILE} not present")
