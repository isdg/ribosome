#ifndef RIBOSOME_GATE_H
#define RIBOSOME_GATE_H

#include <stdint.h>

typedef uint8_t bit;

bit nand(bit a, bit b);

bit _not(bit a);
bit _and(bit a, bit b);
bit _or(bit a, bit b);
bit _xor(bit a, bit b);
bit mux(bit a, bit b, bit sel);  /* ternary */

uint64_t gate_count(void);
void gate_count_reset(void);

#endif
