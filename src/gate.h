#ifndef RIBOSOME_GATE_H
#define RIBOSOME_GATE_H

#include <stdint.h>

typedef uint8_t bit;

bit nand(bit a, bit b);

bit not_(bit a);
bit and_(bit a, bit b);
bit or_(bit a, bit b);
bit xor_(bit a, bit b);
bit mux(bit a, bit b, bit sel);  /* ternary */

uint64_t gate_count(void);
void gate_count_reset(void);

#endif
