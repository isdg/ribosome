#include <cstdint>
#include "gate.h"
#define BUS_MAX 32

void bus_from_u32(uint32_t v, bit *out, int n);
uint32_t bus_to_u32(const bit *in, int n);
void bus_copy(const bit *src, bit *dst, int n);
