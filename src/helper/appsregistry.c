#include "appsregistry.h"

uint8_t appsCount = 0;
uint8_t appsToRunCount = 0;

App *apps[256] = {
    
};
App *appsAvailableToRun[256];

void APPS_Register(App *app) {
  apps[appsCount++] = app;
  if (app->runnable) {
    appsAvailableToRun[appsToRunCount++] = app;
  }
}

void APPS_RegisterAll(void) {}
