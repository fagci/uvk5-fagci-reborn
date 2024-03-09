#ifndef APPSREGISTRY_H
#define APPSREGISTRY_H

#include "../globals.h"
#include <stdint.h>

App *APPS_GetById(AppType_t id);
void APPS_Register(App *app);
void APPS_RegisterAll();

extern App *apps[256];
extern App *appsAvailableToRun[256];
extern uint8_t appsCount;
extern uint8_t appsToRunCount;

#endif /* end of include guard: APPSREGISTRY_H */
