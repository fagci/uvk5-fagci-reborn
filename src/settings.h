#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

typedef struct {
  uint8_t busyChannelTxLock : 1;
} Settings;

// 0..6400 channels
// 6400..6528 list names
// 6528..6544 currentVfo

#endif /* end of include guard: SETTINGS_H */
