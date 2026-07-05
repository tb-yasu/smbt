"""save() then load() must preserve results; loaded index reports its mode."""

import smbt
from conftest import MODES, make_dataset


def test_save_load_roundtrip(tmp_path):
    data = make_dataset(n_rows=200, seed=4)
    for mode in MODES:
        idx = smbt.build(data, mode=mode, minsup=3)
        path = tmp_path / f"idx{mode}.smbt"
        idx.save(path)

        loaded = smbt.load(path)
        assert loaded.mode == mode
        for q in ([0, 1, 2, 3], [10, 20, 30], [7, 8]):
            assert loaded.search(q, 0.5) == idx.search(q, 0.5)


def test_load_reports_mode(tmp_path):
    data = make_dataset(n_rows=50, seed=5)
    for mode in MODES:
        p = tmp_path / f"m{mode}.smbt"
        smbt.build(data, mode=mode, minsup=2).save(p)
        assert smbt.load(p).mode == mode
