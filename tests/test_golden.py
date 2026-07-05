"""Full-dataset golden test: build dat/fingerprints.dat, run the first 200
fingerprints as queries at similarity 0.9, and match test/expected_q200_s090.txt
(the same golden the shell regression checks). Skipped when the sample data is
not present (e.g. an sdist-only install)."""

import os

import pytest

import smbt
from conftest import DATA_FILE, GOLDEN_FILE, MODES, requires_data_file, requires_golden

N_QUERIES = 200
SIMILARITY = 0.9


def _read_fingerprints(path):
    fps = []
    with open(path) as f:
        for line in f:
            items = [int(x) for x in line.split()]
            if items:
                fps.append(items)
    return fps


def _canonical(qid, hits):
    # Mirror the CLI's output pipeline exactly: smbt-search prints each 32-bit
    # float similarity with std::cout's default 6-significant-digit format, and
    # the regression's normalize() then does round(float(that_text), 4). The id
    # sets are identical regardless; this only pins down the 4th-decimal rounding
    # of the rare value that sits on a boundary.
    pairs = sorted((int(i), round(float("%.6g" % float(s)), 4)) for (i, s) in hits)
    return "query id:%d num:%d %s" % (
        qid, len(pairs), " ".join("%d:%.4f" % p for p in pairs))


@pytest.mark.skipif(
    os.environ.get("SMBT_SKIP_GOLDEN") == "1", reason="SMBT_SKIP_GOLDEN=1"
)
@pytest.mark.parametrize("mode", MODES)
def test_golden(mode):
    requires_data_file()
    requires_golden()

    fps = _read_fingerprints(DATA_FILE)
    expected = GOLDEN_FILE.read_text().splitlines()
    assert len(expected) == N_QUERIES

    idx = smbt.build(fps, mode=mode, minsup=10)
    for qid in range(N_QUERIES):
        got = _canonical(qid, idx.search(fps[qid], similarity=SIMILARITY))
        assert got == expected[qid], f"mode {mode} query {qid}\n got: {got}\n exp: {expected[qid]}"
