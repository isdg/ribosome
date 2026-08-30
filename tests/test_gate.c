/* tests/test_gate.c - exhaustive verification of src/gate.c
 *
 * Every function in gate.c takes at most three bits, so this suite is
 * *total*: it covers every reachable input state rather than sampling.
 *
 * Two tiers, and both earn their place:
 *
 *   truth tables  - is the logic right?
 *   gate counts   - is it actually built out of NANDs?
 *
 * Only the second tier catches `return a ^ b;`, which passes every
 * correctness test ever written while quietly turning the structural
 * core into an ordinary emulator.
 *
 * Cost is asserted per input, not once per gate. A combinational circuit
 * evaluates the same gates whatever the inputs are, so an input-dependent
 * count means something is short-circuiting.
 *
 * The oracles below use host operators (&&, ||, !=). That is legal *here*
 * precisely because it is banned in src/ - the host is the independent
 * second opinion, and it stays independent only if the two never share an
 * implementation.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "gate.h"

static int checks = 0;
static int failures = 0;

#define CHECK(cond, ...)   \
  do {                     \
    checks++;              \
    if (!(cond)) {         \
      failures++;          \
      printf("  FAIL  ");  \
      printf(__VA_ARGS__); \
      printf("\n");        \
    }                      \
  } while (0)

/* ---- oracles: the host's own answer, computed independently -------- */

static bit o_nand(bit a, bit b) { return (bit)!(a && b); }
static bit o_and(bit a, bit b) { return (bit)(a && b); }
static bit o_or(bit a, bit b) { return (bit)(a || b); }
static bit o_xor(bit a, bit b) { return (bit)(a != b); }

/* ---- the checks ---------------------------------------------------- */

typedef bit (*gate2)(bit, bit);

static void check_gate2(const char *name, gate2 f, gate2 oracle,
                        uint64_t cost) {
  for (bit a = 0; a < 2; a++) {
    for (bit b = 0; b < 2; b++) {
      gate_count_reset();
      bit got = f(a, b);
      uint64_t used = gate_count();
      bit want = oracle(a, b);

      CHECK(got == want, "%s(%u,%u) = %u, want %u",
            name, (unsigned)a, (unsigned)b, (unsigned)got, (unsigned)want);
      CHECK(used == cost, "%s(%u,%u) cost %" PRIu64 ", want %" PRIu64,
            name, (unsigned)a, (unsigned)b, used, cost);
    }
  }
}

static void check_not(void) {
  for (bit a = 0; a < 2; a++) {
    gate_count_reset();
    bit got = _not(a);
    uint64_t used = gate_count();
    bit want = (bit)!a;

    CHECK(got == want, "_not(%u) = %u, want %u",
          (unsigned)a, (unsigned)got, (unsigned)want);
    CHECK(used == 1, "_not(%u) cost %" PRIu64 ", want 1", (unsigned)a, used);
  }
}

static void check_mux(void) {
  for (bit sel = 0; sel < 2; sel++) {
    for (bit a = 0; a < 2; a++) {
      for (bit b = 0; b < 2; b++) {
        gate_count_reset();
        bit got = mux(a, b, sel);
        uint64_t used = gate_count();
        bit want = sel ? b : a;

        CHECK(got == want, "mux(%u,%u,sel=%u) = %u, want %u",
              (unsigned)a, (unsigned)b, (unsigned)sel, (unsigned)got,
              (unsigned)want);
        CHECK(used == 4, "mux(%u,%u,sel=%u) cost %" PRIu64 ", want 4",
              (unsigned)a, (unsigned)b, (unsigned)sel, used);
      }
    }
  }
}

int main(void) {
  check_gate2("nand", nand, o_nand, 1);
  check_gate2("_and", _and, o_and, 2);
  check_gate2("_or", _or, o_or, 3);
  check_gate2("_xor", _xor, o_xor, 4);
  check_not();
  check_mux();

  printf("gate: %d checks, %d failures\n", checks, failures);
  return failures != 0;
}
