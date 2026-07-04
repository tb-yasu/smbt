#!/usr/bin/env bash
#
# smbt regression suite.
#
# Usage: test/run_regression.sh [--byte-gate DIR] [--keep]
#
#   --byte-gate DIR   also compare freshly built idx1.bin/idx2.bin/idx3.bin
#                     (built from dat/fingerprints.dat) against
#                     DIR/idx1.bin, DIR/idx2.bin, DIR/idx3.bin. The mode-3
#                     comparison ignores the `dim` field at byte offset 16
#                     (8 bytes): that field goes from uninitialized garbage
#                     to a deterministic zero once the MBT constructor fix
#                     is applied, so it is intentionally excluded.
#   --keep            do not delete the temporary working directory on
#                     exit; its path is printed so a failing case can be
#                     inspected.
#
# Env overrides: SMBT_BUILD, SMBT_SEARCH (paths to the two binaries;
# default to prog/smbt-build and prog/smbt-search under the repo root).
#
set -u -o pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_BIN="${SMBT_BUILD:-$ROOT/prog/smbt-build}"
SEARCH_BIN="${SMBT_SEARCH:-$ROOT/prog/smbt-search}"
DATA="$ROOT/dat/fingerprints.dat"
GOLDEN="$ROOT/test/expected_q200_s090.txt"

BYTE_GATE_DIR=""
KEEP=0
while [ $# -gt 0 ]; do
  case "$1" in
    --byte-gate) BYTE_GATE_DIR="$2"; shift 2 ;;
    --keep) KEEP=1; shift ;;
    -h|--help)
      sed -n '2,20p' "$0"
      exit 0
      ;;
    *) echo "unknown option: $1" >&2; exit 2 ;;
  esac
done

if [ ! -x "$BUILD_BIN" ] || [ ! -x "$SEARCH_BIN" ]; then
  echo "error: $BUILD_BIN / $SEARCH_BIN not found or not executable (run 'make' first)" >&2
  exit 2
fi
if [ ! -f "$DATA" ] || [ ! -f "$GOLDEN" ]; then
  echo "error: expected $DATA and $GOLDEN to exist" >&2
  exit 2
fi

WORK="$(mktemp -d "${TMPDIR:-/tmp}/smbt_regress.XXXXXX")"
cleanup() {
  if [ "$KEEP" -eq 1 ]; then
    echo "kept working directory: $WORK"
  else
    rm -rf "$WORK"
  fi
}
trap cleanup EXIT

NPASS=0
NFAIL=0
FAILED=()

pass() { echo "  PASS  $1"; NPASS=$((NPASS + 1)); }
fail() { echo "  FAIL  $1"; NFAIL=$((NFAIL + 1)); FAILED+=("$1"); }
section() { echo; echo "== $1 =="; }

# run_with_timeout SECS CMD...  -- portable-ish wrapper around GNU/BSD
# `timeout`/`gtimeout`, falling back to a manual background+kill if neither
# is installed.
run_with_timeout() {
  local secs="$1"; shift
  if command -v timeout >/dev/null 2>&1; then
    timeout "$secs" "$@"
  elif command -v gtimeout >/dev/null 2>&1; then
    gtimeout "$secs" "$@"
  else
    "$@" &
    local pid=$!
    ( sleep "$secs" 2>/dev/null && kill -9 "$pid" 2>/dev/null ) &
    local watcher=$!
    wait "$pid" 2>/dev/null
    local rc=$?
    kill "$watcher" 2>/dev/null
    wait "$watcher" 2>/dev/null
    return $rc
  fi
}

# is_crash RC -- true if RC looks like a signal death (segfault/abort/OOM
# kill) rather than a normal exit() call.
is_crash() { [ "$1" -ge 128 ]; }

# assert_clean_failure LABEL RC -- the process must have exited with a
# nonzero, non-crashing, non-timeout status.
assert_clean_failure() {
  local label="$1" rc="$2"
  if [ "$rc" -eq 124 ]; then
    fail "$label: timed out / hung (rc=124)"
  elif [ "$rc" -eq 0 ]; then
    fail "$label: exited 0 (expected a clear error)"
  elif is_crash "$rc"; then
    fail "$label: crashed (rc=$rc)"
  else
    pass "$label: exits cleanly (rc=$rc)"
  fi
}

# normalize SEARCH_STDOUT_FILE -- print a canonical form of smbt-search's
# "query id:N num:K id:sim ..." lines: pairs sorted by id, sim rounded to
# 4 decimals, so outputs from different modes/tree-shapes are comparable.
normalize() {
  python3 - "$1" <<'PYEOF'
import re, sys

LINE_RE = re.compile(r'^query id:(\d+) num:(\d+)\s*(.*)$')

def normalize_lines(lines):
    out = []
    for line in lines:
        line = line.rstrip('\n')
        if not line.startswith('query id:'):
            continue
        m = LINE_RE.match(line)
        if not m:
            raise ValueError('unparseable query line: %r' % line)
        qid = int(m.group(1))
        rest = m.group(3).strip()
        pairs = []
        if rest:
            for tok in rest.split():
                pid_s, sim_s = tok.split(':')
                pairs.append((int(pid_s), round(float(sim_s), 4)))
        pairs.sort()
        out.append('query id:%d num:%d %s' % (
            qid, len(pairs), ' '.join('%d:%.4f' % p for p in pairs)))
    return out

with open(sys.argv[1]) as f:
    for l in normalize_lines(f):
        print(l)
PYEOF
}

build_ok() {
  # build_ok MODE INFILE OUTIDX [MINSUP]  -- returns smbt-build's exit code
  local mode="$1" infile="$2" outidx="$3" minsup="${4:-10}"
  "$BUILD_BIN" -mode "$mode" -minsup "$minsup" "$infile" "$outidx" \
    > "$WORK/build.$mode.stdout" 2> "$WORK/build.$mode.stderr"
}

search_ok() {
  # search_ok IDXFILE QFILE OUTSTDOUT [SIM] -- returns smbt-search's exit code
  local idxfile="$1" qfile="$2" outstdout="$3" sim="${4:-0.9}"
  "$SEARCH_BIN" -similarity "$sim" "$idxfile" "$qfile" \
    > "$outstdout" 2> "$WORK/search.stderr"
}

##############################################################################
section "0. rsdic property test (test/rsdic_check.cpp)"
##############################################################################
RSDIC_LIB="$ROOT/src/rsdic/lib"
if [ ! -f "$RSDIC_LIB/EnumCoder.o" ] || [ ! -f "$RSDIC_LIB/RSDic.o" ] || [ ! -f "$RSDIC_LIB/RSDicBuilder.o" ]; then
  ( cd "$RSDIC_LIB" && make ) > "$WORK/rsdic_lib_build.log" 2>&1
fi
if [ ! -f "$RSDIC_LIB/EnumCoder.o" ] || [ ! -f "$RSDIC_LIB/RSDic.o" ] || [ ! -f "$RSDIC_LIB/RSDicBuilder.o" ]; then
  fail "rsdic property test: $RSDIC_LIB/*.o missing and could not be built (see $WORK/rsdic_lib_build.log)"
else
  if g++ -O3 -DNDEBUG -Wno-deprecated -c -o "$WORK/rsdic_check.o" "$ROOT/test/rsdic_check.cpp" \
       > "$WORK/rsdic_check_compile.log" 2>&1 \
     && g++ -O3 -DNDEBUG -o "$WORK/rsdic_check" "$WORK/rsdic_check.o" \
       "$RSDIC_LIB/EnumCoder.o" "$RSDIC_LIB/RSDic.o" "$RSDIC_LIB/RSDicBuilder.o" -lm \
       >> "$WORK/rsdic_check_compile.log" 2>&1; then
    if "$WORK/rsdic_check" > "$WORK/rsdic_check.stdout" 2> "$WORK/rsdic_check.stderr"; then
      pass "rsdic property test: $(cat "$WORK/rsdic_check.stdout")"
    else
      fail "rsdic property test reported failures: $(cat "$WORK/rsdic_check.stdout")"
      sed 's/^/    /' "$WORK/rsdic_check.stderr"
    fi
  else
    fail "rsdic property test: compile failed (see $WORK/rsdic_check_compile.log)"
  fi
fi

##############################################################################
section "1. cross-mode + golden agreement"
##############################################################################
head -n 200 "$DATA" > "$WORK/query200.dat"
ok=1
for m in 1 2 3; do
  if ! build_ok "$m" "$DATA" "$WORK/idx_m$m.bin"; then
    fail "mode $m: build failed (see $WORK/build.$m.stderr)"; ok=0; continue
  fi
  if ! search_ok "$WORK/idx_m$m.bin" "$WORK/query200.dat" "$WORK/search_m$m.stdout"; then
    fail "mode $m: search failed"; ok=0; continue
  fi
  normalize "$WORK/search_m$m.stdout" > "$WORK/norm_m$m.txt"
done
if [ "$ok" -eq 1 ]; then
  if diff -q "$WORK/norm_m1.txt" "$WORK/norm_m2.txt" >/dev/null \
     && diff -q "$WORK/norm_m2.txt" "$WORK/norm_m3.txt" >/dev/null; then
    pass "mode1 == mode2 == mode3"
  else
    fail "mode1/mode2/mode3 results differ from each other"
  fi
  if diff -q "$WORK/norm_m3.txt" "$GOLDEN" >/dev/null; then
    pass "mode3 == golden ($GOLDEN)"
  else
    fail "mode3 result does not match golden"
  fi
  for m in 1 2; do
    if diff -q "$WORK/norm_m$m.txt" "$GOLDEN" >/dev/null; then
      pass "mode$m == golden"
    else
      fail "mode$m result does not match golden"
    fi
  done
fi

##############################################################################
section "2. shuffled input (per-line token order scrambled)"
##############################################################################
python3 - "$DATA" "$WORK/shuffled.dat" <<'PYEOF'
import random, sys
random.seed(12345)
inp, outp = sys.argv[1], sys.argv[2]
with open(inp) as f, open(outp, 'w') as o:
    for line in f:
        toks = line.split()
        random.shuffle(toks)
        o.write(' '.join(toks) + '\n')
PYEOF
head -n 200 "$WORK/shuffled.dat" > "$WORK/shuffled_query200.dat"
for m in 1 2 3; do
  if ! build_ok "$m" "$WORK/shuffled.dat" "$WORK/idx_shuf_m$m.bin"; then
    fail "mode $m: build on shuffled input failed"; continue
  fi
  if ! search_ok "$WORK/idx_shuf_m$m.bin" "$WORK/shuffled_query200.dat" "$WORK/search_shuf_m$m.stdout"; then
    fail "mode $m: search on shuffled input failed"; continue
  fi
  normalize "$WORK/search_shuf_m$m.stdout" > "$WORK/norm_shuf_m$m.txt"
  if diff -q "$WORK/norm_shuf_m$m.txt" "$GOLDEN" >/dev/null; then
    pass "mode $m: shuffled-input result == golden (order-independence holds)"
  else
    fail "mode $m: shuffled-input result != golden (likely relies on sorted rows)"
  fi
done

##############################################################################
section "3. mode-3 (MBT) build determinism"
##############################################################################
build_ok 3 "$DATA" "$WORK/det1.bin"
build_ok 3 "$DATA" "$WORK/det2.bin"
if cmp -s "$WORK/det1.bin" "$WORK/det2.bin"; then
  pass "two mode-3 builds from the same input produce byte-identical index"
else
  fail "two mode-3 builds from the same input DIFFER (uninitialized field?)"
fi

##############################################################################
if [ -n "$BYTE_GATE_DIR" ]; then
section "4. byte-identical gate vs $BYTE_GATE_DIR"
##############################################################################
  for m in 1 2; do
    if [ ! -f "$BYTE_GATE_DIR/idx$m.bin" ]; then
      fail "mode $m: baseline $BYTE_GATE_DIR/idx$m.bin not found"
      continue
    fi
    if cmp -s "$WORK/idx_m$m.bin" "$BYTE_GATE_DIR/idx$m.bin"; then
      pass "mode $m: index byte-identical to baseline"
    else
      fail "mode $m: index DIFFERS from baseline"
    fi
  done
  if [ -f "$BYTE_GATE_DIR/idx3.bin" ]; then
    if python3 - "$WORK/idx_m3.bin" "$BYTE_GATE_DIR/idx3.bin" <<'PYEOF'
import sys
a = bytearray(open(sys.argv[1], 'rb').read())
b = bytearray(open(sys.argv[2], 'rb').read())
DIM_OFF, DIM_LEN = 16, 8
if len(a) != len(b):
    sys.exit(1)
a[DIM_OFF:DIM_OFF + DIM_LEN] = b'\x00' * DIM_LEN
b[DIM_OFF:DIM_OFF + DIM_LEN] = b'\x00' * DIM_LEN
sys.exit(0 if a == b else 1)
PYEOF
    then
      pass "mode 3: index byte-identical to baseline (ignoring dim field)"
    else
      fail "mode 3: index DIFFERS from baseline (beyond the dim field)"
    fi
  else
    fail "mode 3: baseline $BYTE_GATE_DIR/idx3.bin not found"
  fi
fi

##############################################################################
section "5. CLI edge cases"
##############################################################################

# -- nonexistent index file: must fail fast, no hang/OOM --------------------
run_with_timeout 10 "$SEARCH_BIN" -similarity 0.9 "$WORK/does_not_exist.bin" "$WORK/query200.dat" \
  > "$WORK/s_noidx.stdout" 2> "$WORK/s_noidx.stderr"
assert_clean_failure "search on nonexistent index" "$?"

# -- trailing option with a missing value: must not segfault ----------------
run_with_timeout 10 "$BUILD_BIN" -mode \
  > "$WORK/trail_mode.stdout" 2> "$WORK/trail_mode.stderr"
assert_clean_failure "smbt-build trailing -mode" "$?"

run_with_timeout 10 "$BUILD_BIN" -mode 1 -minsup \
  > "$WORK/trail_minsup.stdout" 2> "$WORK/trail_minsup.stderr"
assert_clean_failure "smbt-build trailing -minsup" "$?"

run_with_timeout 10 "$SEARCH_BIN" -similarity \
  > "$WORK/trail_sim.stdout" 2> "$WORK/trail_sim.stderr"
assert_clean_failure "smbt-search trailing -similarity" "$?"

# -- invalid -mode value / unreadable input: no garbage index left behind ---
rm -f "$WORK/badmode_idx.bin"
run_with_timeout 10 "$BUILD_BIN" -mode 9 -minsup 10 "$DATA" "$WORK/badmode_idx.bin" \
  > "$WORK/badmode.stdout" 2> "$WORK/badmode.stderr"
assert_clean_failure "invalid -mode 9" "$?"
if [ -e "$WORK/badmode_idx.bin" ]; then
  fail "invalid -mode 9 left a garbage index file behind"
else
  pass "invalid -mode 9 leaves no index file behind"
fi

rm -f "$WORK/unreadable_idx.bin"
run_with_timeout 10 "$BUILD_BIN" -mode 1 -minsup 10 "$WORK/no_such_input.dat" "$WORK/unreadable_idx.bin" \
  > "$WORK/unreadable.stdout" 2> "$WORK/unreadable.stderr"
assert_clean_failure "unreadable input file" "$?"
if [ -e "$WORK/unreadable_idx.bin" ]; then
  fail "unreadable input file left a garbage index file behind"
else
  pass "unreadable input file leaves no index file behind"
fi

# -- truncated index file: must fail cleanly, not hang/OOM/segfault --------
if [ -f "$WORK/idx_m1.bin" ]; then
  fullsize=$(wc -c < "$WORK/idx_m1.bin" | tr -d ' ')
  half=$((fullsize / 2))
  head -c "$half" "$WORK/idx_m1.bin" > "$WORK/idx_truncated.bin"
  run_with_timeout 10 "$SEARCH_BIN" -similarity 0.9 "$WORK/idx_truncated.bin" "$WORK/query200.dat" \
    > "$WORK/trunc.stdout" 2> "$WORK/trunc.stderr"
  assert_clean_failure "truncated index file" "$?"
else
  fail "truncated-index check skipped (no baseline idx_m1.bin from section 1)"
fi

# -- old-format (v1) index: rejected with a clear rebuild message -----------
if [ -f "$WORK/idx_m1.bin" ]; then
  cp "$WORK/idx_m1.bin" "$WORK/idx_v1.bin"
  printf '\x01\x00\x00\x00\x00\x00\x00\x00' | dd of="$WORK/idx_v1.bin" bs=1 count=8 conv=notrunc 2>/dev/null
  run_with_timeout 10 "$SEARCH_BIN" -similarity 0.9 "$WORK/idx_v1.bin" "$WORK/query200.dat" \
    > "$WORK/v1fmt.stdout" 2> "$WORK/v1fmt.stderr"
  assert_clean_failure "old-format (v1) index" "$?"
  if grep -q "old index format (v1)" "$WORK/v1fmt.stderr"; then
    pass "old-format (v1) index: stderr tells the user to rebuild"
  else
    fail "old-format (v1) index: stderr lacks the rebuild message"
  fi
else
  fail "v1-format check skipped (no baseline idx_m1.bin from section 1)"
fi

# -- empty input file: clear error, not a crash on fvs[0] -------------------
: > "$WORK/empty.dat"
for m in 1 2 3; do
  rm -f "$WORK/empty_idx$m.bin"
  run_with_timeout 10 "$BUILD_BIN" -mode "$m" -minsup 10 "$WORK/empty.dat" "$WORK/empty_idx$m.bin" \
    > "$WORK/empty_build$m.stdout" 2> "$WORK/empty_build$m.stderr"
  assert_clean_failure "mode $m: build on empty input" "$?"
  if [ -e "$WORK/empty_idx$m.bin" ]; then
    fail "mode $m: empty input left a garbage index file behind"
  fi
done

# -- single-query search must not print NaN ---------------------------------
if [ -f "$WORK/idx_m1.bin" ]; then
  head -n 1 "$DATA" > "$WORK/query1.dat"
  search_ok "$WORK/idx_m1.bin" "$WORK/query1.dat" "$WORK/nan_check.stdout"
  if grep -qi 'nan' "$WORK/nan_check.stdout" 2>/dev/null; then
    fail "single-query search prints NaN"
  else
    pass "single-query search does not print NaN"
  fi
else
  fail "NaN check skipped (no baseline idx_m1.bin from section 1)"
fi

# -- blank / whitespace-only lines are ignored, not treated as a fingerprint
head -n 300 "$DATA" > "$WORK/ws_base.dat"
head -n 30 "$DATA" > "$WORK/ws_query.dat"
python3 - "$WORK/ws_base.dat" "$WORK/ws_with_blank.dat" <<'PYEOF'
import sys
inp, outp = sys.argv[1], sys.argv[2]
with open(inp) as f:
    lines = f.readlines()
lines.insert(5, "   \n")
with open(outp, 'w') as f:
    f.writelines(lines)
PYEOF
for m in 1 2 3; do
  if ! build_ok "$m" "$WORK/ws_base.dat" "$WORK/ws_base_idx$m.bin" 3; then
    fail "mode $m: build on ws_base failed"; continue
  fi
  if ! build_ok "$m" "$WORK/ws_with_blank.dat" "$WORK/ws_blank_idx$m.bin" 3; then
    fail "mode $m: build on ws_with_blank failed"; continue
  fi
  search_ok "$WORK/ws_base_idx$m.bin" "$WORK/ws_query.dat" "$WORK/ws_base_search$m.stdout"
  search_ok "$WORK/ws_blank_idx$m.bin" "$WORK/ws_query.dat" "$WORK/ws_blank_search$m.stdout"
  normalize "$WORK/ws_base_search$m.stdout" > "$WORK/ws_base_norm$m.txt"
  normalize "$WORK/ws_blank_search$m.stdout" > "$WORK/ws_blank_norm$m.txt"
  if diff -q "$WORK/ws_base_norm$m.txt" "$WORK/ws_blank_norm$m.txt" >/dev/null; then
    pass "mode $m: blank/whitespace-only line does not change results"
  else
    fail "mode $m: blank/whitespace-only line changes results (consuming an id?)"
  fi
done

# -- duplicate values within a line == deduplicated line --------------------
head -n 50 "$DATA" > "$WORK/dup_base.dat"
head -n 20 "$DATA" > "$WORK/dup_query.dat"
python3 - "$WORK/dup_base.dat" "$WORK/dup_with_dup.dat" <<'PYEOF'
import sys
inp, outp = sys.argv[1], sys.argv[2]
with open(inp) as f:
    lines = f.readlines()
toks = lines[2].split()
toks.append(toks[0])
lines[2] = ' '.join(toks) + '\n'
with open(outp, 'w') as f:
    f.writelines(lines)
PYEOF
for m in 1 2 3; do
  if ! build_ok "$m" "$WORK/dup_base.dat" "$WORK/dup_base_idx$m.bin" 3; then
    fail "mode $m: build on dup_base failed"; continue
  fi
  if ! build_ok "$m" "$WORK/dup_with_dup.dat" "$WORK/dup_dup_idx$m.bin" 3; then
    fail "mode $m: build on dup_with_dup failed"; continue
  fi
  search_ok "$WORK/dup_base_idx$m.bin" "$WORK/dup_query.dat" "$WORK/dup_base_search$m.stdout"
  search_ok "$WORK/dup_dup_idx$m.bin" "$WORK/dup_query.dat" "$WORK/dup_dup_search$m.stdout"
  normalize "$WORK/dup_base_search$m.stdout" > "$WORK/dup_base_norm$m.txt"
  normalize "$WORK/dup_dup_search$m.stdout" > "$WORK/dup_dup_norm$m.txt"
  if diff -q "$WORK/dup_base_norm$m.txt" "$WORK/dup_dup_norm$m.txt" >/dev/null; then
    pass "mode $m: duplicate value within a line does not change results"
  else
    fail "mode $m: duplicate value within a line changes results"
  fi
done

##############################################################################
section "6. build time / index size (informational)"
##############################################################################
for m in 1 2 3; do
  if [ -f "$WORK/idx_m$m.bin" ]; then
    sz=$(wc -c < "$WORK/idx_m$m.bin" | tr -d ' ')
    echo "  mode $m: index size = $sz bytes"
    grep -E "construction time|building time|total building time" "$WORK/build.$m.stdout" 2>/dev/null | sed 's/^/    /'
  fi
done

##############################################################################
echo
echo "================================================================"
echo "  TOTAL: $NPASS passed, $NFAIL failed"
if [ "$NFAIL" -gt 0 ]; then
  echo "  FAILED CHECKS:"
  for f in "${FAILED[@]}"; do echo "    - $f"; done
fi
echo "================================================================"
[ "$NFAIL" -eq 0 ]
