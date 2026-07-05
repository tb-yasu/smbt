"""Building from an in-memory list and from the equivalent (blank-line-free)
file must produce a byte-identical index, and identical search results."""

import smbt
from conftest import MODES, make_dataset


def _write_fp_file(path, data):
    with open(path, "w") as f:
        for row in data:
            f.write(" ".join(str(x) for x in row) + "\n")


def test_inmemory_vs_file_byte_identical(tmp_path):
    data = make_dataset(n_rows=250, seed=3)
    fp_file = tmp_path / "data.dat"
    _write_fp_file(fp_file, data)

    for mode in MODES:
        mem_idx = smbt.build(data, mode=mode, minsup=3)
        file_idx = smbt.build_file(fp_file, mode=mode, minsup=3)

        mem_path = tmp_path / f"mem{mode}.smbt"
        file_path = tmp_path / f"file{mode}.smbt"
        mem_idx.save(mem_path)
        file_idx.save(file_path)

        assert mem_path.read_bytes() == file_path.read_bytes(), f"mode {mode} index differs"

        # and results match
        for q in ([0, 1, 2], [5, 6, 7, 8], [39]):
            assert mem_idx.search(q, 0.5) == file_idx.search(q, 0.5)
