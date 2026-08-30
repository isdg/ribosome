/* tests/test_bus.c - the marshalling boundary.
 *
 * bus.c moves values between host integers and wires. It computes
 * nothing, so the load-bearing assertion here is not any single value:
 * it is that gate_count() never moves. The moment marshalling costs a
 * gate, a circuit has leaked into the boundary layer and the ALU's cost
 * numbers stop meaning anything.
 *
 * The width sweep matters as much as the values. Every bus function is
 * parameterized on n so the same netlist serves both the exhaustive
 * 8-bit ALU test and the real 32-bit machine; if n is mishandled here,
 * that whole strategy quietly stops working.
 */

#include <inttypes.h>
#include <stdint.h>

#include "bus.h"
#include "check.h"
#include "gate.h"

/* Deterministic xorshift32: a fixed seed means a failure reproduces. */
static uint32_t rng_state = 0x13579bdfu;

static uint32_t rng(void) {
  uint32_t x = rng_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return rng_state = x;
}

/* The low n bits of v - what a bus of width n must carry.
 * The ternary is not paranoia: 1u << 32 is undefined in C. */
static uint32_t trunc_to(uint32_t v, int n) {
  return n == 32 ? v : v & ((1u << n) - 1u);
}

static const uint32_t EDGES[] = {
    0u, 1u, 2u, 0x7fffffffu, 0x80000000u, 0xfffffffeu, 0xffffffffu,
    0xa5a5a5a5u, 0xdeadbeefu,
};

#define NEDGES ((int)(sizeof EDGES / sizeof EDGES[0]))

/* ---- round trips ---------------------------------------------------- */

static void check_roundtrip8(void) {
  bit b[8];

  for (uint32_t v = 0; v < 256u; v++) {
    gate_count_reset();
    bus_from_u32(v, b, 8);
    uint32_t got = bus_to_u32(b, 8);
    uint64_t used = gate_count();

    CHECK(got == v, "n=8: 0x%02" PRIx32 " -> 0x%02" PRIx32, v, got);
    CHECK(used == 0, "n=8: 0x%02" PRIx32 " cost %" PRIu64 ", want 0", v,
          used);
  }
}

static void check_edges(void) {
  bit b[BUS_MAX];

  for (int i = 0; i < NEDGES; i++) {
    gate_count_reset();
    bus_from_u32(EDGES[i], b, BUS_MAX);
    uint32_t got = bus_to_u32(b, BUS_MAX);
    uint64_t used = gate_count();

    /* 0x80000000 is the one that catches `in[i] << i` promoting to int
     * and overflowing at bit 31 - undefined, and UBSan will say so. */
    CHECK(got == EDGES[i], "edge 0x%08" PRIx32 " -> 0x%08" PRIx32,
          EDGES[i], got);
    CHECK(used == 0, "edge 0x%08" PRIx32 " cost %" PRIu64 ", want 0",
          EDGES[i], used);
  }
}

/* Every width, including the ones that truncate. */
static void check_widths(void) {
  bit b[BUS_MAX];

  for (int n = 1; n <= BUS_MAX; n++) {
    for (int k = 0; k < 256; k++) {
      uint32_t v = rng();
      uint32_t want = trunc_to(v, n);

      bus_from_u32(v, b, n);
      uint32_t got = bus_to_u32(b, n);

      CHECK(got == want,
            "n=%d: 0x%08" PRIx32 " -> 0x%08" PRIx32 ", want 0x%08" PRIx32,
            n, v, got, want);
    }
  }
}

/* ---- structure ------------------------------------------------------ */

/* Bit i of the value must land at index i, and nowhere else. */
static void check_placement(void) {
  bit b[BUS_MAX];

  for (int i = 0; i < BUS_MAX; i++) {
    bus_from_u32(1u << i, b, BUS_MAX);

    for (int j = 0; j < BUS_MAX; j++) {
      bit want = (bit)(j == i);
      CHECK(b[j] == want, "1<<%d: b[%d] = %u, want %u", i, j,
            (unsigned)b[j], (unsigned)want);
    }
  }
}

/* A wire carries 0 or 1. Anything else corrupts every gate downstream,
 * and only nand()'s assert would notice - in debug builds only. */
static void check_bits_are_bits(void) {
  bit b[BUS_MAX];

  for (int k = 0; k < 256; k++) {
    uint32_t v = rng();
    bus_from_u32(v, b, BUS_MAX);

    for (int i = 0; i < BUS_MAX; i++)
      CHECK(b[i] <= 1, "0x%08" PRIx32 ": b[%d] = %u, want 0 or 1", v, i,
            (unsigned)b[i]);
  }
}

static void check_copy(void) {
  bit src[BUS_MAX], dst[BUS_MAX];

  bus_from_u32(0xdeadbeefu, src, BUS_MAX);

  gate_count_reset();
  bus_copy(src, dst, BUS_MAX);
  uint64_t used = gate_count();

  CHECK(bus_to_u32(dst, BUS_MAX) == 0xdeadbeefu, "bus_copy: dst wrong");
  CHECK(bus_to_u32(src, BUS_MAX) == 0xdeadbeefu, "bus_copy: src clobbered");
  CHECK(used == 0, "bus_copy cost %" PRIu64 ", want 0", used);
}

/* Copying a narrow bus must not touch the wires above it. */
static void check_copy_width(void) {
  bit src[BUS_MAX], dst[BUS_MAX];

  bus_from_u32(0xffffffffu, src, BUS_MAX);
  bus_from_u32(0u, dst, BUS_MAX);
  bus_copy(src, dst, 8);

  CHECK(bus_to_u32(dst, BUS_MAX) == 0xffu,
        "bus_copy n=8 wrote 0x%08" PRIx32 ", want 0x000000ff",
        bus_to_u32(dst, BUS_MAX));
}

int main(void) {
  check_roundtrip8();
  check_edges();
  check_widths();
  check_placement();
  check_bits_are_bits();
  check_copy();
  check_copy_width();

  return check_summary("bus");
}
