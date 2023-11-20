#include "savech.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../radio.h"
#include "../ui/components.h"
#include "../ui/helper.h"
#include "apps.h"
#include <string.h>

static uint16_t currentChannelIndex = 0;

static char channelNames[CHANNELS_COUNT][16];
static bool gotChannelNames = false;
static uint16_t readIndex = 0;

void SAVECH_init() { gotChannelNames = false; }
void SAVECH_update() {
  if (!gotChannelNames) {
    EEPROM_ReadBuffer(VFO_SIZE * readIndex + 8 + CHANNELS_OFFSET,
                      channelNames[readIndex], 15);
    if (UI_NoChannelName(channelNames[readIndex])) {
      sprintf(channelNames[readIndex], "CH-%03u", readIndex + 1);
    }
    if ((readIndex & 7) == 0) {
      gRedrawScreen = true;
    }
    if (++readIndex >= CHANNELS_COUNT) {
      gRedrawScreen = true;
      gotChannelNames = true;
    }
  }
}
bool SAVECH_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed) {
    return false;
  }
  switch (key) {
  case KEY_UP:
    currentChannelIndex =
        currentChannelIndex == 0 ? CHANNELS_COUNT - 1 : currentChannelIndex - 1;
    gRedrawScreen = true;
    return true;
  case KEY_DOWN:
    currentChannelIndex =
        currentChannelIndex == CHANNELS_COUNT - 1 ? 0 : currentChannelIndex + 1;
    gRedrawScreen = true;
    return true;
  case KEY_MENU:
    strncpy(channelNames[currentChannelIndex], gCurrentVfo.name, 15);
    EEPROM_WriteBuffer(VFO_SIZE * currentChannelIndex + CHANNELS_OFFSET,
                       &gCurrentVfo, VFO_SIZE);
    gRedrawScreen = true;
    return true;
  case KEY_EXIT:
    APPS_run(gPreviousApp);
    return true;
  default:
    break;
  }
  return false;
}

void SAVECH_render() {
  char String[16];
  UI_ClearScreen();
  if (gotChannelNames) {
    UI_ShowItems(channelNames, CHANNELS_COUNT, currentChannelIndex);
    sprintf(String, "%u", currentChannelIndex + 1);
    UI_PrintStringSmall(String, 0, 0, 6);
  } else {
    char pb[] = "-\\|/";
    char String[2] = {0};
    UI_PrintString("Reading", 4, 4, 0);
    UI_PrintString("channels", 4, 4, 2);
    sprintf(String, "%c", pb[readIndex & 3]);
    UI_PrintString(String, 72, 72, 2);
  }
}
