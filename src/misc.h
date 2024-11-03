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

char IsPrintable(char ch);

#define MHZ 100000

#endif /* end of include guard: MISC_H */
