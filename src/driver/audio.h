#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <stdint.h>

void AUDIO_ToggleSpeaker(bool on);

// Do not use when receiving (IDK why yet)
void AUDIO_PlayTone(uint32_t frequency, uint16_t duration);

#endif
