/*
 * rsdic_check.cpp
 *
 * Property test for the vendored src/rsdic rank/select dictionary.
 * rsdic itself is not modified by this file or anywhere else in this
 * change set; this only exercises its existing public contract
 * (GetBit/Rank/Select/Save+Load) against a naive reference and a
 * deterministic PRNG, since the bundled unit tests (waf + python2) do not
 * run in this environment.
 *
 * Not part of the smbt-build/smbt-search binaries: compiled standalone by
 * test/run_regression.sh, linked directly against the already-built
 * object files under src/rsdic/lib/.
 */

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>
#include <stdint.h>

#include "../src/rsdic/lib/RSDicBuilder.hpp"
#include "../src/rsdic/lib/RSDic.hpp"

using namespace std;

static int g_failures = 0;

#define CHECK(cond, msg, label) \
  do { \
    if (!(cond)) { \
      fprintf(stderr, "FAIL: %s [%s] (%s:%d)\n", msg, label, __FILE__, __LINE__); \
      g_failures++; \
    } \
  } while (0)

// Deterministic xorshift64 PRNG: libc rand()'s sequence isn't guaranteed
// stable across platforms, and reproducibility matters more here than
// statistical quality.
class Rng {
public:
  Rng(uint64_t seed) : state_(seed ? seed : 0x9E3779B97F4A7C15ULL) {}
  uint64_t next() {
    state_ ^= state_ << 13;
    state_ ^= state_ >> 7;
    state_ ^= state_ << 17;
    return state_;
  }
  // true with probability numerator/1000
  bool chance(uint64_t numerator) {
    return (next() % 1000) < numerator;
  }
private:
  uint64_t state_;
};

static vector<bool> makeRandomBits(uint64_t len, uint64_t densityPermil, Rng &rng) {
  vector<bool> bits(len);
  for (uint64_t i = 0; i < len; ++i)
    bits[i] = rng.chance(densityPermil);
  return bits;
}

static void checkOne(const vector<bool> &bits, const char *label) {
  uint64_t len = bits.size();

  rsdic::RSDicBuilder builder;
  for (uint64_t i = 0; i < len; ++i)
    builder.PushBack(bits[i]);
  rsdic::RSDic rs;
  builder.Build(rs);

  CHECK(rs.num() == len, "num() matches pushed length", label);

  uint64_t ones = 0;
  for (uint64_t i = 0; i < len; ++i)
    if (bits[i]) ++ones;
  CHECK(rs.one_num() == ones, "one_num() matches naive count", label);

  uint64_t rank1 = 0;
  for (uint64_t i = 0; i < len; ++i) {
    CHECK((bool)rs.GetBit(i) == bits[i], "GetBit matches naive bit", label);
    CHECK(rs.Rank(i, 1) == rank1, "Rank(i,1) matches naive prefix count", label);
    CHECK(rs.Rank(i, 0) == i - rank1, "Rank(i,0) matches naive prefix count", label);
    if (bits[i]) ++rank1;
  }
  CHECK(rs.Rank(len, 1) == rank1, "Rank(len,1) == total ones", label);
  CHECK(rs.Rank(len, 0) == len - rank1, "Rank(len,0) == total zeros", label);

  {
    uint64_t seen = 0;
    for (uint64_t i = 0; i < len; ++i) {
      if (bits[i]) {
        CHECK(rs.Select(seen, 1) == i, "Select(ind,1) matches naive scan", label);
        ++seen;
      }
    }
  }
  {
    uint64_t seen = 0;
    for (uint64_t i = 0; i < len; ++i) {
      if (!bits[i]) {
        CHECK(rs.Select(seen, 0) == i, "Select(ind,0) matches naive scan", label);
        ++seen;
      }
    }
  }

  {
    ostringstream oss;
    rs.Save(oss);
    string data = oss.str();
    istringstream iss(data);
    rsdic::RSDic loaded;
    loaded.Load(iss);
    CHECK(loaded == rs, "Save/Load round trip preserves equality", label);
    CHECK(loaded.num() == rs.num(), "Save/Load round trip preserves num()", label);
  }
}

int main() {
  uint64_t lengths[] = {1, 63, 64, 65, 1023, 1024, 1025, 2047, 2048, 2049, 100000};
  uint64_t densities[] = {0, 10, 500, 990, 1000}; // 0%, 1%, 50%, 99%, 100% (permille)

  Rng rng(0xC0FFEEULL);
  int cases = 0;
  for (size_t li = 0; li < sizeof(lengths) / sizeof(lengths[0]); ++li) {
    for (size_t di = 0; di < sizeof(densities) / sizeof(densities[0]); ++di) {
      uint64_t len = lengths[li];
      uint64_t density = densities[di];
      vector<bool> bits = makeRandomBits(len, density, rng);
      char label[128];
      snprintf(label, sizeof(label), "len=%llu density=%llu/1000",
               (unsigned long long)len, (unsigned long long)density);
      checkOne(bits, label);
      ++cases;
    }
  }

  {
    // Alternating 0/1 pattern spanning several large/small block boundaries:
    // stresses Select's block-scan differently than uniform random bits.
    vector<bool> bits(4096);
    for (size_t i = 0; i < bits.size(); ++i)
      bits[i] = (i % 2 == 0);
    checkOne(bits, "alternating 0/1, len=4096");
    ++cases;
  }

  if (g_failures == 0) {
    printf("rsdic_check: OK (%d cases, 0 failed assertions)\n", cases);
    return 0;
  }
  printf("rsdic_check: FAILED (%d cases, %d failed assertions)\n", cases, g_failures);
  return 1;
}
