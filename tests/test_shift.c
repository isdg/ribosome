/* tests/test_shift.c - barrel shifter against native C.
 * Exhaustive at n=8, every shamt at n=32, cost asserted per input.
 * shamt wires past log2(n) hold UNSET, so reading them trips nand(). */

#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include "bus.h"
#include "check.h"
#include "gate.h"
#include "shift.h"

#define MUX_COST 4
#define UNSET 2

/* _and(_and(arith, _not(left)), msb): the sign copy, zero for SLL. */
#define FILL_COST 5

static uint32_t rng_state = 0x9e3779b9u;

static uint32_t rng(void) {
  uint32_t x = rng_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return rng_state = x;
}

static uint32_t low_bits(int n) {
  return n == 32 ? 0xffffffffu : (1u << n) - 1u;
}

static int log2n(int n) {
  int k = 0;
  while ((1 << k) < n)
    k++;
  return k;
}

static const uint32_t EDGES[] = {
    0u,          1u,          0x7fffffffu, 0x80000000u, 0x80000001u,
    0xfffffffeu, 0xffffffffu, 0x55555555u, 0xaaaaaaaau, 0xdeadbeefu,
};

#define NEDGES ((int)(sizeof EDGES / sizeof EDGES[0]))

static const char *const OP[2][2] = {{"srl", "sra"}, {"sll", "sll+arith"}};

/* ---- oracles -------------------------------------------------------- */

/* x is already truncated to n bits and s < n, so no shift is UB. */
static uint32_t oracle_shr(uint32_t x, int s, bit fill, int n) {
  uint32_t m = low_bits(n);
  uint32_t vacated = m & ~(m >> s);
  return (x >> s) | (fill ? vacated : 0u);
}

static uint32_t oracle_shift(uint32_t x, int s, bit left, bit arith, int n) {
  if (left)
    return (x << s) & low_bits(n);

  bit sign = (bit)((x >> (n - 1)) & 1u);
  return oracle_shr(x, s, (bit)(arith && sign), n);
}

/* ---- runners -------------------------------------------------------- */

struct shift_out {
  uint32_t value;
  uint64_t cost;
};

static void load(uint32_t x, int s, int n, bit *wx, bit *ws, bit *wo) {
  bus_from_u32(x, wx, n);
  memset(ws, UNSET, BUS_MAX);
  bus_from_u32((uint32_t)s, ws, log2n(n));
  memset(wo, UNSET, BUS_MAX);
}

static struct shift_out run_shr(uint32_t x, int s, bit fill, int n) {
  bit wx[BUS_MAX], ws[BUS_MAX], wo[BUS_MAX];
  struct shift_out r;

  load(x, s, n, wx, ws, wo);
  gate_count_reset();
  shr(wx, ws, fill, wo, n);
  r.cost = gate_count();
  r.value = bus_to_u32(wo, n);
  return r;
}

static struct shift_out run_shift(uint32_t x, int s, bit left, bit arith,
                                  int n) {
  bit wx[BUS_MAX], ws[BUS_MAX], wo[BUS_MAX];
  struct shift_out r;

  load(x, s, n, wx, ws, wo);
  gate_count_reset();
  shift(wx, ws, left, arith, wo, n);
  r.cost = gate_count();
  r.value = bus_to_u32(wo, n);
  return r;
}

/* ---- shr ------------------------------------------------------------ */

static void check_shr(uint32_t x, int s, bit fill, int n) {
  x &= low_bits(n);

  uint32_t want = oracle_shr(x, s, fill, n);
  uint64_t want_cost = (uint64_t)MUX_COST * (uint64_t)(n * log2n(n));
  struct shift_out r = run_shr(x, s, fill, n);

  CHECK(r.value == want,
        "shr n=%d 0x%" PRIx32 ">>%d fill %u: 0x%" PRIx32 ", want 0x%" PRIx32, n,
        x, s, fill, r.value, want);
  CHECK(r.cost == want_cost,
        "shr n=%d 0x%" PRIx32 ">>%d fill %u: cost %" PRIu64 ", want %" PRIu64,
        n, x, s, fill, r.cost, want_cost);
}

static void check_shr8_exhaustive(void) {
  for (uint32_t x = 0; x < 256u; x++)
    for (int s = 0; s < 8; s++)
      for (bit fill = 0; fill <= 1; fill++)
        check_shr(x, s, fill, 8);
}

static void check_shr_widths(void) {
  for (int n = 2; n <= BUS_MAX; n *= 2)
    for (int s = 0; s < n; s++)
      for (int k = 0; k < 16; k++)
        check_shr(rng(), s, (bit)(k & 1), n);
}

static void check_shr32_edges(void) {
  for (int i = 0; i < NEDGES; i++)
    for (int s = 0; s < 32; s++)
      for (bit fill = 0; fill <= 1; fill++)
        check_shr(EDGES[i], s, fill, 32);
}

/* ---- shift ---------------------------------------------------------- */

static void check_shift(uint32_t x, int s, bit left, bit arith, int n) {
  x &= low_bits(n);

  uint32_t want = oracle_shift(x, s, left, arith, n);
  uint64_t want_cost =
      (uint64_t)MUX_COST * (uint64_t)(n * (log2n(n) + 2)) + FILL_COST;
  struct shift_out r = run_shift(x, s, left, arith, n);
  const char *op = OP[left][arith];

  CHECK(r.value == want,
        "%s n=%d 0x%" PRIx32 " by %d: 0x%" PRIx32 ", want 0x%" PRIx32, op, n, x,
        s, r.value, want);
  CHECK(r.cost == want_cost,
        "%s n=%d 0x%" PRIx32 " by %d: cost %" PRIu64 ", want %" PRIu64, op, n,
        x, s, r.cost, want_cost);
}

static void check_shift8_exhaustive(void) {
  for (uint32_t x = 0; x < 256u; x++)
    for (int s = 0; s < 8; s++)
      for (bit left = 0; left <= 1; left++)
        for (bit arith = 0; arith <= 1; arith++)
          check_shift(x, s, left, arith, 8);
}

static void check_shift_widths(void) {
  for (int n = 2; n <= BUS_MAX; n *= 2)
    for (int s = 0; s < n; s++)
      for (int k = 0; k < 16; k++)
        check_shift(rng(), s, (bit)(k & 1), (bit)((k >> 1) & 1), n);
}

static void check_shift32_edges(void) {
  for (int i = 0; i < NEDGES; i++)
    for (int s = 0; s < 32; s++)
      for (bit left = 0; left <= 1; left++)
        for (bit arith = 0; arith <= 1; arith++)
          check_shift(EDGES[i], s, left, arith, 32);
}

static void check_shift32_random(void) {
  for (int k = 0; k < 4096; k++) {
    uint32_t r = rng();
    check_shift(rng(), (int)(r & 31u), (bit)((r >> 5) & 1u),
                (bit)((r >> 6) & 1u), 32);
  }
}

/* ---- structure ------------------------------------------------------ */

static void check_width_bound(void) {
  bit wx[BUS_MAX], ws[BUS_MAX], wo[BUS_MAX];

  load(0xa5u, 3, 8, wx, ws, wo);
  shr(wx, ws, 1, wo, 8);
  for (int i = 8; i < BUS_MAX; i++)
    CHECK(wo[i] == UNSET, "shr n=8 wrote out[%d] = %u", i, wo[i]);

  load(0xa5u, 3, 8, wx, ws, wo);
  shift(wx, ws, 1, 0, wo, 8);
  for (int i = 8; i < BUS_MAX; i++)
    CHECK(wo[i] == UNSET, "shift n=8 wrote out[%d] = %u", i, wo[i]);
}

/* In place: every stage reads wires an earlier stage may overwrite. */
static void check_alias(void) {
  for (int k = 0; k < 256; k++) {
    uint32_t x = rng(), r = rng();
    int s = (int)(r & 31u);
    bit left = (bit)((r >> 5) & 1u), arith = (bit)((r >> 6) & 1u);
    bit wx[BUS_MAX], ws[BUS_MAX], wo[BUS_MAX];

    load(x, s, 32, wx, ws, wo);
    shr(wx, ws, arith, wx, 32);
    uint32_t got = bus_to_u32(wx, 32);
    uint32_t want = oracle_shr(x, s, arith, 32);
    CHECK(got == want,
          "alias shr 0x%" PRIx32 ">>%d: 0x%" PRIx32 ", want 0x%" PRIx32, x, s,
          got, want);

    load(x, s, 32, wx, ws, wo);
    shift(wx, ws, left, arith, wx, 32);
    got = bus_to_u32(wx, 32);
    want = oracle_shift(x, s, left, arith, 32);
    CHECK(got == want,
          "alias %s 0x%" PRIx32 " by %d: 0x%" PRIx32 ", want 0x%" PRIx32,
          OP[left][arith], x, s, got, want);
  }
}

int main(void) {
  check_shr8_exhaustive();
  check_shr_widths();
  check_shr32_edges();
  check_shift8_exhaustive();
  check_shift_widths();
  check_shift32_edges();
  check_shift32_random();
  check_width_bound();
  check_alias();

  return check_summary("shift");
}
