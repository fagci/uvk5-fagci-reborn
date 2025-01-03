#ifndef MISC_H
#define MISC_H

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#define MAKE_WORD(hb, lb) (((uint8_t)(hb) << 8U) | (uint8_t)lb)
#define SWAP(a, b)                                                             \
  {                                                                            \
    __typeof__(a) t = (a);                                                     \
    a = b;                                                                     \
    b = t;                                                                     \
  }

#ifndef MIN
#define MIN(a, b)                                                              \
  ({                                                                           \
    __typeof__(a) _a = (a);                                                    \
    __typeof__(b) _b = (b);                                                    \
    _a < _b ? _a : _b;                                                         \
  })
#endif

char IsPrintable(char ch);
unsigned int SQRT16(unsigned int value);

#define MHZ 100000

#endif /* end of include guard: MISC_H */
