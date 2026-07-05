"""The three modes must return identical results, and those results must match
a brute-force Jaccard oracle."""

import pytest

import smbt
from conftest import MODES, brute_force, jaccard, make_dataset

QUERIES = [
    [0, 1, 2, 3],
    [5, 10, 15, 20, 25],
    [7],
    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
    [38, 39],
]
SIMS = [0.3, 0.5, 0.7, 0.9]


def _results(idx, query, sim):
    return {int(i): float(s) for (i, s) in idx.search(query, similarity=sim)}


@pytest.mark.parametrize("sim", SIMS)
@pytest.mark.parametrize("query", QUERIES)
def test_modes_agree_and_match_oracle(dataset, query, sim):
    per_mode = {}
    for mode in MODES:
        idx = smbt.build(dataset, mode=mode, minsup=3)
        per_mode[mode] = _results(idx, query, sim)

    # 1) all three modes return the same id set
    id_sets = {mode: set(r.keys()) for mode, r in per_mode.items()}
    assert id_sets[1] == id_sets[2] == id_sets[3], (query, sim, id_sets)

    # 2) similarities agree across modes (same float math -> exact)
    for mode in (2, 3):
        for i, s in per_mode[mode].items():
            assert per_mode[1][i] == s

    # 3) match brute force with a tolerance band around the threshold to avoid
    #    float boundary flakiness
    got = per_mode[1]
    q = set(query)
    for i, fp in enumerate(dataset):
        s = jaccard(q, fp)
        if s >= sim + 1e-6:
            assert i in got, f"row {i} (sim={s}) should be a hit"
        elif s <= sim - 1e-6:
            assert i not in got, f"row {i} (sim={s}) should not be a hit"
    # every returned similarity is close to the true Jaccard
    for i, s in got.items():
        assert abs(s - jaccard(q, dataset[i])) < 1e-5


def test_results_sorted_desc():
    data = make_dataset(n_rows=200, seed=1)
    idx = smbt.build(data, mode=1, minsup=2)
    hits = idx.search([0, 1, 2, 3, 4], similarity=0.1)
    sims = [s for (_i, s) in hits]
    assert sims == sorted(sims, reverse=True)
    # ties broken by ascending id
    for a, b in zip(hits, hits[1:]):
        if a[1] == b[1]:
            assert a[0] < b[0]


def test_unsorted_and_duplicate_query_equivalent():
    data = make_dataset(seed=2)
    idx = smbt.build(data, mode=2, minsup=3)
    a = idx.search([9, 3, 3, 1, 9, 5], similarity=0.4)
    b = idx.search([1, 3, 5, 9], similarity=0.4)
    assert a == b
