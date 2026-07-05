"""verbose=False (the default) must not print anything during build."""

import smbt
from conftest import make_dataset


def test_quiet_by_default(capfd):
    data = make_dataset(n_rows=100, seed=9)
    for mode in (1, 2, 3):
        smbt.build(data, mode=mode, minsup=3)
    out, err = capfd.readouterr()
    assert out == ""
    assert err == ""


def test_verbose_true_prints(capfd):
    data = make_dataset(n_rows=100, seed=10)
    smbt.build(data, mode=1, minsup=3, verbose=True)
    out, err = capfd.readouterr()
    # progress goes to stderr, statistics to stdout; at least something shows up
    assert out != "" or err != ""
