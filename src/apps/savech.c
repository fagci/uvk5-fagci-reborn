#include "savech.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../helper/adapter.h"
#include "../helper/channels.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../helper/vfos.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"
#include "textinput.h"
#include <stdio.h>

static uint16_t currentChannelIndex = 0;
static uint16_t chCount = 0;
static char tempName[9] = {0};

static void getChannelName(uint16_t i, char *name) {
  CH ch;
  CHANNELS_Load(i, &ch);
  if (IsReadable(ch.name)) {
    strncpy(name, ch.name, 31);
  } else {
    sprintf(name, "CH-%u", i + 1);
  }
}

static void saveNamed() {
  CH ch;
  VFO2CH(gCurrentVFO, &ch);
  strncpy(ch.name, tempName, 9);
  CHANNELS_Save(currentChannelIndex, &ch);
}

void SAVECH_init() { chCount = CHANNELS_GetCountMax(); }
void SAVECH_update() {}

bool SAVECH_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  CH ch;
  switch (key) {
  case KEY_UP:
    IncDec16(&currentChannelIndex, 0, chCount, -1);
    return true;
  case KEY_DOWN:
    IncDec16(&currentChannelIndex, 0, chCount, 1);
    return true;
  case KEY_MENU:
    gTextinputText = tempName;
    snprintf(gTextinputText, 9, "%lu.%lu", gCurrentVFO->fRX / 100000,
             gCurrentVFO->fRX % 100000);
    gTextInputSize = 9;
    gTextInputCallback = saveNamed;
    APPS_run(APP_TEXTINPUT);
    return true;
  case KEY_EXIT:
    APPS_exit();
    return true;
  case KEY_0:
    CHANNELS_Delete(currentChannelIndex);
    return true;
  case KEY_PTT:
    CHANNELS_Load(currentChannelIndex, &ch);
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
