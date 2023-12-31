#include "savech.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../helper/channels.h"
#include "../helper/measurements.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"
#include <stdio.h>

static uint16_t currentChannelIndex = 0;
static uint16_t chCount = 0;

static void getChannelName(uint16_t i, char *name) {
  VFO ch;
  CHANNELS_LoadUser(i, &ch);
  if (IsReadable(ch.name)) {
    strncpy(name, ch.name, 31);
  } else {
    sprintf(name, "CH-%u", i + 1);
  }
}

void SAVECH_init() { chCount = CHANNELS_GetCountMax(); }
void SAVECH_update() {}

bool SAVECH_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  VFO ch;
  switch (key) {
  case KEY_UP:
    IncDec16(&currentChannelIndex, 0, chCount, -1);
    return true;
  case KEY_DOWN:
    IncDec16(&currentChannelIndex, 0, chCount, 1);
    return true;
  case KEY_MENU:
    CHANNELS_SaveCurrentVFO(currentChannelIndex);
    return true;
  case KEY_EXIT:
    APPS_exit();
    return true;
  case KEY_0:
    CHANNELS_Delete(currentChannelIndex);
    return true;
  case KEY_PTT:
    CHANNELS_LoadUser(currentChannelIndex, &ch);
    RADIO_TuneToSave(ch.fRX);
    APPS_run(APP_STILL);
    return true;
  default:
    break;
  }
  return false;
}

void SAVECH_render() {
  UI_ClearScreen();
  UI_ShowMenu(getChannelName, chCount, currentChannelIndex);
}
