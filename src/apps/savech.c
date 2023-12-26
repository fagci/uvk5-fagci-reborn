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
  char *chName = ch.name;
  if (chName[0] > 32 && chName[0] < 127) {
    strncpy(name, chName, 31);
  } else {
    sprintf(name, "CH-%u", i + 1);
  }
}

void SAVECH_init() { chCount = CHANNELS_GetCountMax(); }
void SAVECH_update() {}

bool SAVECH_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  switch (key) {
  case KEY_UP:
    IncDec16(&currentChannelIndex, 0, chCount, -1);
    return true;
  case KEY_DOWN:
    IncDec16(&currentChannelIndex, 0, chCount, 1);
    return true;
  case KEY_MENU:
    /* strncpy(channelNames[currentChannelIndex], gCurrentVFO->name, 15);
    EEPROM_WriteBuffer(VFO_SIZE * currentChannelIndex + CHANNELS_OFFSET,
                       &gCurrentVFO, VFO_SIZE);
    gRedrawScreen = true; */
    return true;
  case KEY_EXIT:
    APPS_exit();
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
