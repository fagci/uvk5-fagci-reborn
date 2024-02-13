#ifndef NUMNAV_H
#define NUMNAV_H

#include "../driver/keyboard.h"
#include <stdint.h>

extern bool gIsNumNavInput;
extern char gNumNavInput[16];
extern void (*gNumNavCallback)(uint16_t result);

void NUMNAV_Init(uint16_t initialValue, uint16_t min, uint16_t max);
uint16_t NUMNAV_Input(KEY_Code_t key);
void NUMNAV_Accept(void);
uint16_t NUMNAV_GetCurrentValue(void);
uint16_t NUMNAV_Deinit(void);

#endif /* end of include guard: NUMNAV_H */
