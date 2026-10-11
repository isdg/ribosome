#ifndef RIBOSOME_ADDER_H
#define RIBOSOME_ADDER_H

#include "gate.h"

void half_add(bit a, bit b, bit *sum, bit *carry);
void full_add(bit a, bit b, bit cin, bit *sum, bit *cout);

/* Ripple-carry over n >= 1 wires, LSB first; sum may alias a or b.
 * cin_msb is the carry into bit n-1: overflow = cout XOR cin_msb. */
void add(const bit *a, const bit *b, bit cin, bit *sum, int n, bit *cout,
         bit *cin_msb);

#endif
