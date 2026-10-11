/* tests/test_adder.c - half, full and ripple adders against native C.
 * Exhaustive at n=8, edges and random at n=32, cost asserted per input.
 * Outputs start at 2, an impossible wire value, so unwritten ones show. */

#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include "adder.h"
#include "bus.h"
#include "check.h"
#include "gate.h"

#define HALF_COST 6
#define FULL_COST 15
#define UNSET 2

static uint32_t rng_state = 0x2468ace1u;

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

static const uint32_t EDGES[] = {
    0u,          1u,          2u,          0x7fffffffu, 0x80000000u,
    0x80000001u, 0xfffffffeu, 0xffffffffu, 0x55555555u, 0xaaaaaaaau,
};

#define NEDGES ((int)(sizeof EDGES / sizeof EDGES[0]))

/* ---- one-bit adders ------------------------------------------------- */

static void check_half(void) {
  for (bit a = 0; a <= 1; a++) {
    for (bit b = 0; b <= 1; b++) {
      bit sum = UNSET, carry = UNSET;
      unsigned want = (unsigned)a + b;

      gate_count_reset();
      half_add(a, b, &sum, &carry);
      uint64_t used = gate_count();

      CHECK(sum == (want & 1u), "half %u+%u: sum %u, want %u", a, b, sum,
            want & 1u);
      CHECK(carry == (want >> 1), "half %u+%u: carry %u, want %u", a, b, carry,
            want >> 1);
      CHECK(used == HALF_COST, "half %u+%u: cost %" PRIu64 ", want %d", a, b,
            used, HALF_COST);
    }
  }
}

static void check_full(void) {
  for (bit a = 0; a <= 1; a++) {
    for (bit b = 0; b <= 1; b++) {
      for (bit c = 0; c <= 1; c++) {
        bit sum = UNSET, cout = UNSET;
        unsigned want = (unsigned)a + b + c;

        gate_count_reset();
        full_add(a, b, c, &sum, &cout);
        uint64_t used = gate_count();

        CHECK(sum == (want & 1u), "full %u+%u+%u: sum %u, want %u", a, b, c,
              sum, want & 1u);
        CHECK(cout == (want >> 1), "full %u+%u+%u: cout %u, want %u", a, b, c,
              cout, want >> 1);
        CHECK(used == FULL_COST, "full %u+%u+%u: cost %" PRIu64 ", want %d", a,
              b, c, used, FULL_COST);
      }
    }
  }
}

/* ---- n-bit ripple adder --------------------------------------------- */

struct sum_out {
  uint32_t sum;
  bit cout;
  bit cin_msb;
  uint64_t cost;
};

static struct sum_out run_add(uint32_t a, uint32_t b, bit cin, int n) {
  bit wa[BUS_MAX], wb[BUS_MAX], ws[BUS_MAX];
  struct sum_out r = {0, UNSET, UNSET, 0};

  bus_from_u32(a, wa, n);
  bus_from_u32(b, wb, n);
  memset(ws, UNSET, sizeof ws);

  gate_count_reset();
  add(wa, wb, cin, ws, n, &r.cout, &r.cin_msb);
  r.cost = gate_count();
  r.sum = bus_to_u32(ws, n);
  return r;
}

/* The oracle: 64-bit host addition, then read the carries off it. */
static void check_add(uint32_t a, uint32_t b, bit cin, int n) {
  a &= low_bits(n);
  b &= low_bits(n);

  uint64_t full = (uint64_t)a + b + cin;
  uint32_t below = low_bits(n - 1);
  uint32_t into_msb = ((a & below) + (b & below) + cin) >> (n - 1);

  uint32_t want_sum = (uint32_t)full & low_bits(n);
  unsigned want_cout = (unsigned)(full >> n) & 1u;
  unsigned want_cin_msb = into_msb & 1u;
  uint64_t want_cost = (uint64_t)FULL_COST * (uint64_t)n;

  struct sum_out r = run_add(a, b, cin, n);

  CHECK(r.sum == want_sum,
        "n=%d 0x%" PRIx32 "+0x%" PRIx32 "+%u: sum 0x%" PRIx32
        ", want 0x%" PRIx32,
        n, a, b, cin, r.sum, want_sum);
  CHECK(r.cout == want_cout,
        "n=%d 0x%" PRIx32 "+0x%" PRIx32 "+%u: cout %u, want %u", n, a, b, cin,
        r.cout, want_cout);
  CHECK(r.cin_msb == want_cin_msb,
        "n=%d 0x%" PRIx32 "+0x%" PRIx32 "+%u: cin_msb %u, want %u", n, a, b,
        cin, r.cin_msb, want_cin_msb);
  CHECK(r.cost == want_cost,
        "n=%d 0x%" PRIx32 "+0x%" PRIx32 "+%u: cost %" PRIu64 ", want %" PRIu64,
        n, a, b, cin, r.cost, want_cost);
}

static void check_add8_exhaustive(void) {
  for (uint32_t a = 0; a < 256u; a++)
    for (uint32_t b = 0; b < 256u; b++)
      for (bit cin = 0; cin <= 1; cin++)
        check_add(a, b, cin, 8);
}

/* n=1 is the corner where cin_msb is cin itself. */
static void check_add_widths(void) {
  for (int n = 1; n <= BUS_MAX; n++)
    for (int k = 0; k < 64; k++)
      check_add(rng(), rng(), (bit)(rng() & 1u), n);
}

static void check_add32_edges(void) {
  for (int i = 0; i < NEDGES; i++)
    for (int j = 0; j < NEDGES; j++)
      for (bit cin = 0; cin <= 1; cin++)
        check_add(EDGES[i], EDGES[j], cin, 32);
}

static void check_add32_random(void) {
  for (int k = 0; k < 4096; k++)
    check_add(rng(), rng(), (bit)(rng() & 1u), 32);
}

/* ---- structure ------------------------------------------------------ */

static void check_add_width_bound(void) {
  bit wa[BUS_MAX], wb[BUS_MAX], ws[BUS_MAX];
  bit cout, cin_msb;

  bus_from_u32(0xffu, wa, 8);
  bus_from_u32(0x01u, wb, 8);
  memset(ws, UNSET, sizeof ws);
  add(wa, wb, 1, ws, 8, &cout, &cin_msb);

  for (int i = 8; i < BUS_MAX; i++)
    CHECK(ws[i] == UNSET, "add n=8 wrote ws[%d] = %u", i, ws[i]);
}

/* `x += y` in place: ripple order makes this safe, keep it that way. */
static void check_add_alias(void) {
  for (int k = 0; k < 256; k++) {
    uint32_t a = rng(), b = rng();
    uint32_t want = a + b;
    bit wa[BUS_MAX], wb[BUS_MAX];
    bit cout, cin_msb;

    bus_from_u32(a, wa, BUS_MAX);
    bus_from_u32(b, wb, BUS_MAX);
    add(wa, wb, 0, wa, BUS_MAX, &cout, &cin_msb);
    CHECK(bus_to_u32(wa, BUS_MAX) == want,
          "alias sum=a: 0x%" PRIx32 "+0x%" PRIx32 " -> 0x%" PRIx32, a, b,
          bus_to_u32(wa, BUS_MAX));

    bus_from_u32(a, wa, BUS_MAX);
    add(wa, wb, 0, wb, BUS_MAX, &cout, &cin_msb);
    CHECK(bus_to_u32(wb, BUS_MAX) == want,
          "alias sum=b: 0x%" PRIx32 "+0x%" PRIx32 " -> 0x%" PRIx32, a, b,
          bus_to_u32(wb, BUS_MAX));
  }
}

int main(void) {
  check_half();
  check_full();
  check_add8_exhaustive();
  check_add_widths();
  check_add32_edges();
  check_add32_random();
  check_add_width_bound();
  check_add_alias();

  return check_summary("adder");
}
