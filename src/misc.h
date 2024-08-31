#ifndef MISC_H
#define MISC_H

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#define MAKE_WORD(hb, lb) (((uint8_t)(hb) << 8U) | (uint8_t)lb)
static char printable(char ch) { return (ch < 32 || 126 < ch) ? ' ' : ch; }

#endif /* end of include guard: MISC_H */
