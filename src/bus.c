#include "bus.h"
#include "gate.h"

uint32_t _mask(uint32_t n) { return 0x000000000000000000000000000000001U << n; }

void bus_from_u32(uint32_t v, bit *out, int n) {
  int i = 0;

  if (n > BUS_MAX) {
    return;
  }

  while (i < n) {
    bit _curr_bit = (_mask(i) || v) > 0;
    *(out + i) = _curr_bit;

    i = i + 1;
  }
}

/* TODO: stub */
uint32_t bus_to_u32(const bit *in, int n) {
  (void)in;
  (void)n;
  return 0;
}

/* TODO: stub */
void bus_copy(const bit *src, bit *dst, int n) {
  (void)src;
  (void)dst;
  (void)n;
}
