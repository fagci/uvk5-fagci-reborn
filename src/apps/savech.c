#include "savech.h"
#include "../helper/adapter.h"
#include "../helper/channels.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
#include "../helper/bandlist.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "textinput.h"

static uint16_t currentChannelIndex = 0;
static uint16_t chCount = 0;
static char tempName[9] = {0};

static void getChannelName(uint16_t i, char *name) {
  CH ch;
  CHANNELS_Load(i, &ch);
  if (IsReadable(ch.name)) {
    strncpy(name, ch.name, 9);
  } else {
    sprintf(name, "CH-%u", i + 1);
  }
}

static void saveNamed() {
  CH ch;
  CH2CH(radio, gCurrentBand, &ch);
  strncpy(ch.name, tempName, 9);
  CHANNELS_Save(currentChannelIndex, &ch);
  for (uint8_t i = 0; i < 2; ++i) {
    if (gCH[i].channel >= 0 && gCH[i].channel == currentChannelIndex) {
      RADIO_VfoLoadCH(i);
      break;
    }
  }
}

void SAVECH_init() { chCount = CHANNELS_GetCountMax(); }
void SAVECH_update() {}

static void save() {
  gTextinputText = tempName;
  snprintf(gTextinputText, 9, "%lu.%05lu", radio->f / 100000,
           radio->f % 100000);
  gTextInputSize = 9;
  gTextInputCallback = saveNamed;
}

static void setMenuIndexAndRun(uint16_t v) {
  currentChannelIndex = v - 1;
  save();
}

bool SAVECH_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed && !bKeyHeld) {
    if (!gIsNumNavInput && key <= KEY_9) {
      NUMNAV_Init(currentChannelIndex + 1, 1, chCount);
      gNumNavCallback = setMenuIndexAndRun;
    }
    if (gIsNumNavInput) {
      currentChannelIndex = NUMNAV_Input(key) - 1;
      return true;
    }
  }
  CH ch;
  switch (key) {
  case KEY_UP:
    IncDec16(&currentChannelIndex, 0, chCount, -1);
    return true;
  case KEY_DOWN:
    IncDec16(&currentChannelIndex, 0, chCount, 1);
    return true;
  case KEY_MENU:
    save();
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
    RADIO_TuneToSave(ch.f);
    APPS_run(APP_STILL);
    return true;
  default:
    break;
  }
  return false;
}

void SAVECH_render() {
  UI_ClearScreen();
  if (gIsNumNavInput) {
    STATUSLINE_SetText("Select: %s", gNumNavInput);
  }
  UI_ShowMenu(getChannelName, chCount, currentChannelIndex);
}


static App meta = {
    .id = APP_SAVECH,
    .name = "SAVECH",
    .init = SAVECH_init,
    .update = SAVECH_update,
    .render = SAVECH_render,
    .key = SAVECH_key,
};

App *SAVECH_Meta() { return &meta; }
