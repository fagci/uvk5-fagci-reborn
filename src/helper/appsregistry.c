#include "appsregistry.h"

#include "../apps/about.h"
#include "../apps/analyzer.h"
#include "../apps/antenna.h"
#include "../apps/appslist.h"
#include "../apps/bandcfg.h"
#include "../apps/bandlist.h"
#include "../apps/channelscanner.h"
#include "../apps/fastscan.h"
#include "../apps/finput.h"
#include "../apps/lootlist.h"
#include "../apps/multivfo.h"
#include "../apps/reset.h"
#include "../apps/savech.h"
#include "../apps/scanlists.h"
#include "../apps/settings.h"
#include "../apps/spectrumreborn.h"
#include "../apps/still.h"
#include "../apps/taskman.h"
#include "../apps/test.h"
#include "../apps/textinput.h"
#include "../apps/vfocfg.h"

#include "../driver/uart.h"

uint8_t appsCount = 0;
uint8_t appsToRunCount = 0;

App *apps[256];
App *appsAvailableToRun[256];

void APPS_Register(App *app) {
  Log("[+] APPSREGISTRY %s run=%u", app->name, app->runnable);
  apps[appsCount++] = app;
  if (app->runnable) {
    appsAvailableToRun[appsToRunCount++] = app;
  }
}

void APPS_RegisterAll() {
  APPS_Register(FINPUT_Meta());
  APPS_Register(TEXTINPUT_Meta());

  APPS_Register(BANDCFG_Meta());
  APPS_Register(CHCFG_Meta());

  APPS_Register(RESET_Meta());
  APPS_Register(SAVECH_Meta());
  APPS_Register(SETTINGS_Meta());
  APPS_Register(APPSLIST_Meta());

  APPS_Register(STILL_Meta());
  APPS_Register(MULTIVFO_Meta());
  APPS_Register(CHSCANNER_Meta());
  APPS_Register(SPECTRUM_Meta());
  APPS_Register(ANALYZER_Meta());
  APPS_Register(FASTSCAN_Meta());
  APPS_Register(LOOTLIST_Meta());
  APPS_Register(SCANLISTS_Meta());
  APPS_Register(BANDLIST_Meta());
  APPS_Register(ANTENNA_Meta());
  APPS_Register(TASKMAN_Meta());
  APPS_Register(TEST_Meta());
  APPS_Register(ABOUT_Meta());
}
