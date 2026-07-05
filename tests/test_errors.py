"""Error paths map to the right Python exceptions."""

import struct

import pytest

import smbt
from conftest import make_dataset


def test_invalid_mode():
    with pytest.raises(ValueError):
        smbt.build([[1, 2, 3]], mode=9)


def test_empty_dataset():
    with pytest.raises(ValueError):
        smbt.build([], mode=1)


def test_empty_fingerprint_in_data():
    with pytest.raises(ValueError):
        smbt.build([[1, 2, 3], [], [4, 5]], mode=1)


def test_similarity_out_of_range():
    idx = smbt.build(make_dataset(n_rows=20, seed=6), mode=1, minsup=2)
    for bad in (-0.1, 1.5):
        with pytest.raises(ValueError):
            idx.search([1, 2, 3], similarity=bad)


def test_empty_query():
    idx = smbt.build(make_dataset(n_rows=20, seed=7), mode=2, minsup=2)
    with pytest.raises(ValueError):
        idx.search([], similarity=0.5)


def test_load_missing_file(tmp_path):
    with pytest.raises(smbt.Error):
        smbt.load(tmp_path / "does_not_exist.smbt")


def test_load_truncated_index(tmp_path):
    data = make_dataset(n_rows=40, seed=8)
    p = tmp_path / "idx.smbt"
    smbt.build(data, mode=1, minsup=2).save(p)
    raw = p.read_bytes()
    truncated = tmp_path / "trunc.smbt"
    truncated.write_bytes(raw[: len(raw) // 2])
    with pytest.raises(smbt.Error):
        smbt.load(truncated)


def test_load_v1_tag(tmp_path):
    # A v1 index started with the raw mode (1/2/3) instead of mode+10.
    p = tmp_path / "v1.smbt"
    p.write_bytes(struct.pack("<Q", 1) + b"\x00" * 32)
    with pytest.raises(smbt.Error) as exc:
        smbt.load(p)
    assert "rebuild" in str(exc.value).lower()


def test_load_unknown_tag(tmp_path):
    p = tmp_path / "junk.smbt"
    p.write_bytes(struct.pack("<Q", 999) + b"\x00" * 32)
    with pytest.raises(smbt.Error):
        smbt.load(p)


def test_negative_item_id_rejected():
    # uint32 conversion of a negative int must fail, not silently wrap.
    with pytest.raises((TypeError, OverflowError)):
        smbt.build([[1, 2, -3]], mode=1)
