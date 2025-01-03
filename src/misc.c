#include "misc.h"

char IsPrintable(char ch) { return (ch < 32 || 126 < ch) ? ' ' : ch; }

// return square root of 'value'
unsigned int SQRT16(unsigned int value) {
  unsigned int shift = 16; // number of bits supplied in 'value' .. 2 ~ 32
  unsigned int bit = 1u << --shift;
  unsigned int sqrti = 0;
  while (bit) {
    const unsigned int temp = ((sqrti << 1) | bit) << shift--;
    if (value >= temp) {
      value -= temp;
      sqrti |= bit;
    }
    bit >>= 1;
  }
  return sqrti;
}
