#include "savech.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../radio.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"
#include <stdio.h>

static uint16_t currentChannelIndex = 0;

#define CHANNELS_COUNT 255

static char channelNames[CHANNELS_COUNT][16];
static bool gotChannelNames = false;
static uint16_t readIndex = 0;

static void getChannelName(uint16_t i, char *name) {
  strncpy(name, channelNames[i], 31);
}

void SAVECH_init() { gotChannelNames = false; }
void SAVECH_update() {
  if (!gotChannelNames) {
    EEPROM_ReadBuffer(VFO_SIZE * readIndex + 8 + CHANNELS_OFFSET,
                      channelNames[readIndex], 15);
    /* if (UI_NoChannelName(channelNames[readIndex])) {
      sprintf(channelNames[readIndex], "CH-%03u", readIndex + 1);
    } */
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
    strncpy(channelNames[currentChannelIndex], gCurrentVFO->name, 15);
    EEPROM_WriteBuffer(VFO_SIZE * currentChannelIndex + CHANNELS_OFFSET,
                       &gCurrentVFO, VFO_SIZE);
    gRedrawScreen = true;
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
  if (gotChannelNames) {
    UI_ShowMenu(getChannelName, CHANNELS_COUNT, currentChannelIndex);
    PrintMedium(0, 6 * 8 + 12, "%u", currentChannelIndex + 1);
  } else {
    char pb[] = "-\\|/";
    PrintMedium(4, 0 + 12, "Reading");
    PrintMedium(4, 2 * 8 + 12, "channels");
    PrintMedium(72, 2 * 8 + 12, "%c", pb[readIndex & 3]);
  }
}
