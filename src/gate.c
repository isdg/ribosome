#include "gate.h"
#include <assert.h>
#include <stdint.h>

static uint64_t gates = 0;

bit nand(bit a, bit b) {
  static const bit T[2][2] = {{1, 1}, {1, 0}};
  assert(a <= 1 && b <= 1);
  gates = gates + 1;
  return T[a][b];
};

uint64_t gate_count(void) { return gates; }
void gate_count_reset(void) { gates = 0; }
