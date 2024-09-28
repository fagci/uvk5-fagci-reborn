#include "misc.h"

char IsPrintable(char ch) { return (ch < 32 || 126 < ch) ? ' ' : ch; }
