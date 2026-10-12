#ifndef RIBOSOME_SHIFT_H
#define RIBOSOME_SHIFT_H

#include "gate.h"

/* Barrel shift right by shamt (log2 n wires), vacated bits take fill.
 * n is a power of two, 2 <= n <= BUS_MAX; out may alias x. */
void shr(const bit *x, const bit *shamt, bit fill, bit *out, int n);

/* SLL when left = 1, else SRL, or SRA when arith = 1. Left shifts run
 * through shr between two bit-reversal mux rows; arith is ignored. */
void shift(const bit *x, const bit *shamt, bit left, bit arith, bit *out,
           int n);

#endif
