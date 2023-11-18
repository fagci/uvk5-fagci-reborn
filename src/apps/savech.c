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
static uint16_t readIndex = 1;

void SAVECH_init() {
  gotChannelNames = false;
}
void SAVECH_update() {
  if (gotChannelNames) {
    // do nothing
  } else {
    VFO ch;
    EEPROM_ReadBuffer(VFO_SIZE * readIndex, &ch, VFO_SIZE);
    if (UI_NoChannelName(ch.name)) {
      sprintf(channelNames[readIndex], "CH-%03u", readIndex + 1);
    } else {
      strncpy(channelNames[readIndex], ch.name, 15);
      channelNames[readIndex][15] = '\0';
    }
    gRedrawScreen = true;
    if (++readIndex >= CHANNELS_COUNT) {
      gotChannelNames = true;
    }
  }
}
void SAVECH_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed) {
    return;
  }
  switch (key) {
  case KEY_UP:
    currentChannelIndex =
        currentChannelIndex == 0 ? CHANNELS_COUNT - 1 : currentChannelIndex - 1;
    gRedrawScreen = true;
    break;
  case KEY_DOWN:
    currentChannelIndex =
        currentChannelIndex == CHANNELS_COUNT - 1 ? 0 : currentChannelIndex + 1;
    gRedrawScreen = true;
    break;
  case KEY_MENU:
    strncpy(channelNames[currentChannelIndex], gCurrentVfo.name, 15);
    EEPROM_WriteBuffer(VFO_SIZE * currentChannelIndex, &gCurrentVfo);
    gRedrawScreen = true;
    break;
  case KEY_EXIT:
    APPS_run(gPreviousApp);
  default:
    break;
  }
}

void SAVECH_render() {
  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
  if (gotChannelNames) {
    UI_ShowItems(channelNames, CHANNELS_COUNT, currentChannelIndex);
  } else {
    char pb[] = "-\\|/";
    char String[2] = {0};
    UI_PrintString("Reading", 4, 4, 0);
    UI_PrintString("channels", 4, 4, 2);
    sprintf(String, "%c", pb[readIndex & 3]);
    UI_PrintString(String, 72, 72, 2);
  }
}
