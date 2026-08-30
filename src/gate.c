#include "gate.h"
#include <assert.h>
#include <stdint.h>

static uint64_t gates = 0;

// TODO simplify this master
//
bit nand(bit a, bit b) {
  static const bit T[2][2] = {{1, 1}, {1, 0}};
  assert(a <= 1 && b <= 1);
  gates = gates + 1;
  return T[a][b];
};

uint64_t gate_count(void) { return gates; }
void gate_count_reset(void) { gates = 0; }

bit _not(bit a) { return nand(a, a); }

bit _and(bit a, bit b) { return _not(nand(a, b)); }

bit _or(bit a, bit b) { return nand(_not(a), _not(b)); }

bit _xor(bit a, bit b) { return _or(_and(a, b), b); }
