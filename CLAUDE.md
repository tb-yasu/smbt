# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 概要

Succinct Multibit Tree (SMBT) — 化学構造 fingerprint の Jaccard (Tanimoto) 類似度検索の研究用実装(2013 年、C++03 世代)。バイナリは 2 つ: `smbt-build`(index 構築)と `smbt-search`(類似度検索)。

## ビルドと実行

```sh
make            # ルートで実行。サブディレクトリを順にビルドし、バイナリを prog/ へ移動
make clean
```

- 必ずルートの Makefile から: `src/Makefile` は兄弟ディレクトリの .o を直接リンクする。
- Makefile にヘッダ依存関係がないため、**ヘッダ編集後は `make clean && make`**。

```sh
test/run_regression.sh                    # 回帰テスト 30 項目(要: make 済み)
test/run_regression.sh --byte-gate DIR    # 加えて再構築 index を DIR の idx{1,2,3}.bin と byte 比較(33 項目)
```

- 回帰テストの内容: 3 モード相互一致・golden(`test/expected_q200_s090.txt`)比較・シャッフル入力・build 決定性・CLI エッジケース・rsdic property test(`test/rsdic_check.cpp`)。**コードを変更したら必ず実行する**。挙動を変えないはずの修正は `--byte-gate` で index の byte 不変も確認する。
- `src/rsdic/` 付属の unit test は waf + python2 時代の構成で現環境では実行不能(上記 property test が代替)。

```sh
./prog/smbt-build  -mode 1 -minsup 10 dat/fingerprints.dat idx.bin
./prog/smbt-search -similarity 0.9 idx.bin query.dat
```

- オプションは位置引数より**前**に置く(パーサは最初の非 `-` 引数で停止する)。
- 検索結果は stdout に `query id:N num:K id:sim ...`、進捗・統計は混在して stdout/stderr に出る。

## 入力フォーマット(重要)

1 行 = 1 fingerprint、空白区切りの uint32 item id 列(集合として扱われる)。読み込み時に全モードで sort + 重複除去 + 空白のみ行の skip が行われるため、入力順序に正しさは依存しない(昇順・重複なしで用意するのが望ましいのは変わらない)。サンプルは `dat/fingerprints.dat`(9999 行)。

## アーキテクチャ

index ファイルは先頭 uint64 のフォーマットタグ(v2: `mode + 10` = 11/12/13)で始まり、`src/search.cpp` が 3 実装に振り分ける。旧 v1 タグ(1/2/3)は「要再構築」の明示エラーで拒否する(CLI の `-mode` は従来どおり 1/2/3):

| mode | namespace / dir | 内容 |
|------|-----------------|------|
| 1 | `smbt::trie` / `src/smbt_trie/` | succinct multibit tree + succinct trie(DB 本体を LOUDS trie + 差分符号で保持。同一 fingerprint は `unique()` で `clusters_` に統合し代表のみ trie に置く) |
| 2 | `smbt::vla` / `src/smbt_vla/` | succinct multibit tree + 可変長配列(fingerprint ごとに `SucArray`) |
| 3 | `smbt::mbt` / `src/mbt/` | ポインタ版 multibit tree(非圧縮のベースライン) |

3 実装はほぼ同型のコードが別 namespace に複製されている。**read_file・calc_entropy・木構築・検索・統計出力の修正は原則 3 箇所に適用する**。

共通アルゴリズム: fingerprint を cardinality ごとにグループ化して 1 グループ 1 本の木(cardinality 昇順)。各木はエントロピー最大の item による二分割を再帰して構築。内部ノードは oneCols/zeroCols(部分木内で全 1 / 全 0 の列)を持ち、これを累積して Jaccard 上界による admissible な枝刈りを行い、葉では正確な Jaccard を計算する。

**テストに使える不変量**: 枝刈りが admissible で葉が正確計算なので、**検索結果は木の形に依存しない**。同一入力に対し 3 モードの結果集合は完全一致するはず(mode 3 を ground truth にした突き合わせが有効。similarity 0.9・サンプルデータ・先頭 200 行クエリで 4872 件一致を確認済み)。

補助ライブラリ:

- `src/lib/` — すべて `namespace smbt`。`SucArray`(rsdic の select で要素境界を引く自己区切りビット配列)、`VarByte`(差分 + varint。厳密増加列で小さくなるが、任意の uint32 列も mod 2^32 で正しく往復する)、`BitArray`、`checked_read`(index 読込の共通ヘルパ)。
- `src/rsdic/` — **vendored** の rank/select 辞書(岡野原氏作、独自 .git・独自 Makefile を持つ)。**変更しないこと**。`Rank(pos,b)` は [0,pos) の個数、`Select(i,b)` は 0-indexed。`-DNDEBUG` ビルドのため assert は消えており、範囲外アクセスは無防備(呼び出し側が契約を守る)。
- シリアライズは `size_t` を含む生バイナリダンプ(64bit・同一エンディアン前提、magic/version なし)。

## 命名規約(2026-07 適用)

クラス PascalCase、関数・変数 snake_case、class の private メンバは末尾 `_`(struct の public データは `_` なし)、定数 kPascalCase(`kBlockSize`)、namespace は `smbt` / `smbt::{mbt,vla,trie}`、ファイル・ディレクトリ名 snake_case、include guard は `SMBT_..._HPP_`。**例外**: rsdic 呼び出し部(`Rank`/`Select`/`PushBack` 等)は先方の流儀のままで、境界の混在は意図的。新規コードもこの規約に従うこと。

## Python バインディング / PyPI パッケージ(2026-07 追加)

CLI に加えて Python パッケージ `smbt` を提供する。C++ 本体は無改変で共有し、pybind11 で薄くラップする。

- `bindings/smbt_core.cpp` — 単一の `smbt._core.Index` が 3 実装をラップ(mode 1/2/3)。フォーマットタグ(mode+10)は save/load 内で処理(build.cpp/search.cpp を再現)。`smbt::Error`→`smbt.Error`、`std::invalid_argument`→`ValueError`。build/search/save/load で GIL 解放(search は read-only なので並行呼び出し可)。結果は `[(id, sim)]` を sim 降順・id 昇順で返す。
- `python/smbt/__init__.py` — `Index`/`Error` の re-export + `build`/`build_file`/`load` 便宜関数 + `__version__`。
- `setup.py` + `pyproject.toml` — `Pybind11Extension`(src の 9 本 + binding、`-DNDEBUG` 必須、`cxx_std=14`)。バージョンは pyproject 単一ソース(現行 0.1.0)。
- `tests/`(pytest、shell 回帰の `test/` とは別)— 3 モード一致・オラクル・in-memory/ファイル byte 一致・save/load 往復・golden 一致・エラーパス・verbose。
- ビルド: `python setup.py build_ext --inplace` → `PYTHONPATH=python python -m pytest tests/`。配布は cibuildwheel(`.github/workflows/wheels.yml`)。
- C++ 側に追加した API: 各実装に `build_from_data(vector<vector<uint32_t>>, minsup)`(in-memory 入口、空 fingerprint は `invalid_argument`)、`search_fv(fv, sim, res)`(sort+unique して private `search_query` を呼ぶ公開ラッパ)、`set_verbose(bool)`(build 出力を抑制、CLI は default true で出力不変)。同一データならファイル経由と in-memory で index は byte 一致。

## C++ 変更時の不変量(重要)

**C++ 本体(mbt/vla/trie の .cpp/.hpp、build.cpp、search.cpp、lib)を変更したら、必ず `make clean && make` → `test/run_regression.sh --byte-gate ~/Prog/10_cpp/smbt-bytegate-v2`(33/33)を通す**。byte-gate baseline はコード変更前の実バイナリで生成済み。Python 層の変更のみなら pytest で足りるが、binding が参照する C++ シグネチャを変えたら両方回す。

## 既知の残存問題(2026-07 の修正後)

2026-07 に重大バグ(mode 2 の未ソート入力で誤結果、CLI の segfault・暴走、未初期化 `dim` による index 非決定、死んでいたエントロピー early-break 等)を修正し、続く API 整理パス・命名規約パス・リーク解消 + Python バインディングパスを経ている。検索結果の互換性は golden との一致で担保。残存する既知の問題:

- index フォーマットはタグ以外は `size_t` を含む生バイナリダンプのまま(64bit・同一エンディアン前提)。v1 index(2026-07 の API 整理パス以前に構築したもの)は現行バイナリ・Python の両方で読めない(明示エラー)。読む必要があれば下記バックアップの baseline_bin を使う。
- rsdic は `rsdic_uint = uint32_t` のためビット長 2^32 以上で silent wrap。
- rsdic の `Rank(pos, b)` の安全な定義域は `pos ∈ [0, num())`。`Rank(num())` は `num()` が 64 の倍数のとき `rank_small_blocks_[num()/64]` を 1 つ先まで読む OOB(macOS では無害・Linux では segfault)。smbt 本体はノード位置(常に `< num()`)にしか使わないので安全。`test/rsdic_check.cpp` はこの契約を守る(`Rank(len)` は呼ばない)。
- item id は密な次元として扱われるため、巨大 id(例 2^31)はメモリを圧迫する(README にも明記)。
- 修正前スナップショットは `~/Prog/10_cpp/smbt-0.0.1-backup-20260704/`(全体 tar・修正前バイナリ・修正前 v1 index)、rsdic の元 .git は同ディレクトリの `rsdic-dot-git`。**2026-07 にリポジトリを git 化した**(GitHub `tb-yasu/smbt` 公開予定)。fingerprint リークは解消済み(デストラクタ + `free_fvs` + TRIE `unique()` の孤児削除)。
