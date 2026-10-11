#include "bus.h"
#include "gate.h"
#include <stdint.h>

uint32_t _mask(uint32_t n) { return 1U << n; }

void bus_from_u32(uint32_t v, bit *out, int n) {
  int i = 0;

  if (n > BUS_MAX) {
    return;
  }

  while (i < n) {
    bit _curr_bit = (_mask(i) & v) > 0;
    *(out + i) = _curr_bit;

    i = i + 1;
  }
}

/* TODO: stub */
uint32_t bus_to_u32(const bit *in, int n) {
  (void)n;

  uint32_t u32 = 0U;
  int i = 0;

  while (i < n) {
    bit _curr_bit = *(in + i);
    if (_curr_bit) {
      u32 = u32 | (1U >> i);
    }

    i = i + 1;
  }

  return 0;
}

/* TODO: stub */
void bus_copy(const bit *src, bit *dst, int n) {
  (void)src;
  (void)dst;
  (void)n;
}
