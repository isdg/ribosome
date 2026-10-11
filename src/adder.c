#include "adder.h"
#include "bus.h"
#include "gate.h"

void half_add(bit a, bit b, bit *sum, bit *carry) {

  *sum = _xor(a, b);
  *carry = _and(a, b);
}

void full_add(bit a, bit b, bit cin, bit *sum, bit *cout) {
  bit s1;
  bit c1;

  bit c2;

  half_add(a, b, &s1, &c1);
  half_add(s1, cin, sum, &c2);
  *cout = _or(c1, c2);
}

void add(const bit *a, const bit *b, bit cin, bit *sum, int n, bit *cout,
         bit *cin_msb) {
  int i = 0;

  bit c = cin;
  bit _cout = c;
  while (i < n) {
    *cin_msb = _cout;
    full_add(*(a + i), *(b + i), c, (sum + i), &_cout);
    c = _cout;
    i = i + 1;
  }

  *cout = _cout;
}

void addsub(const bit *a, const bit *b, bit sub, bit *sum, int n, bit *cout,
            bit *cin_msb) {
  bit _eff_b[BUS_MAX];
  int i = 0;

  while (i < n) {
    *(_eff_b + i) = _xor(*(b + i), sub);
    i = i + 1;
  }

  add(a, _eff_b, sub, sum, n, cout, cin_msb);
}
