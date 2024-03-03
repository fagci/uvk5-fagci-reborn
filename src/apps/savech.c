#include "savech.h"
#include "../helper/adapter.h"
#include "../helper/channels.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
#include "../helper/presetlist.h"
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

static void saveNamed(void) {
  CH ch;
  VFO2CH(radio, gCurrentPreset, &ch);
  strncpy(ch.name, tempName, 9);
  CHANNELS_Save(currentChannelIndex, &ch);
  for (uint8_t i = 0; i < 2; ++i) {
    if (gVFO[i].channel >= 0 && gVFO[i].channel == currentChannelIndex) {
      RADIO_VfoLoadCH(i);
      break;
    }
  }
}

void SAVECH_init(void) { chCount = CHANNELS_GetCountMax(); }
void SAVECH_update(void) {}

static void save(void) {
  gTextinputText = tempName;
  snprintf(gTextinputText, 9, "%lu.%05lu", radio->rx.f / 100000,
           radio->rx.f % 100000);
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
    RADIO_TuneToSave(ch.rx.f);
    APPS_run(APP_STILL);
    return true;
  default:
    break;
  }
  return false;
}

void SAVECH_render(void) {
  UI_ClearScreen();
  if (gIsNumNavInput) {
    STATUSLINE_SetText("Select: %s", gNumNavInput);
  }
  UI_ShowMenu(getChannelName, chCount, currentChannelIndex);
}

static VFO vfo;

static App meta = {
    .id = APP_SAVECH,
    .name = "SAVECH",
    .init = SAVECH_init,
    .update = SAVECH_update,
    .render = SAVECH_render,
    .key = SAVECH_key,
    .vfo = &vfo,
};

App *SAVECH_Meta(void) { return &meta; }
