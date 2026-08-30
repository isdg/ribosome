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

bit _xor(bit a, bit b) {
  bit c = nand(a, b);
  return nand(nand(a, c), nand(b, c));
}

// a b s | yours | want | gates
// 0 0 0 |   0   |  0   | 4  ok
// 0 1 0 |   0   |  0   | 4  ok
// 1 0 0 |   0   |  1   | 4  FAIL
// 1 1 0 |   0   |  1   | 4  FAIL
// 0 0 1 |   1   |  0   | 4  FAIL
// 0 1 1 |   1   |  1   | 4  ok
// 1 0 1 |   1   |  0   | 4  FAIL
// 1 1 1 |   1   |  1   | 4  ok


bit mux(bit a, bit b, bit sel) {
  bit c = nand(b, sel);
  bit d = nand(a, _not(sel));
  return nand(c, d);
}
