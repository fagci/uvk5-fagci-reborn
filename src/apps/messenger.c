#include "messenger.h"
#include "../apps/textinput.h"
#include "../helper/msghelper.h"
#include "../ui/graphics.h"
#include "apps.h"

static char message[16] = {'\0'};

static void send() { MSG_Send(message); }

void MESSENGER_init() {}

void MESSENGER_update() {}

bool MESSENGER_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  // up-down keys
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    switch (key) {
    case KEY_UP:
      return true;
    case KEY_DOWN:
      return true;
    default:
      break;
    }
  }

  // long held
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
  }

  // Simple keypress
  if (!bKeyPressed && !bKeyHeld) {
    switch (key) {
    case KEY_MENU:
      gTextInputSize = 15;
      gTextinputText = message;
      gTextInputCallback = send;
      APPS_run(APP_TEXTINPUT);
      return true;
    case KEY_EXIT:
      APPS_exit();
      return true;
    default:
      break;
    }
  }
  return false;
}

void MESSENGER_render() {
  UI_ClearScreen();

  uint8_t ii = 0;
  for (uint8_t i = 0; i < 6; ++i) {
    DataPacket *d = MSG_GetMessage(i);
    if (d->data.payload[0] == 0) {
      continue;
    }
    PrintSmallEx(2, LCD_HEIGHT - 2 - ii * 14 - 8, POS_L, C_FILL, "%s",
                 d->data.nick);
    PrintMediumEx(2, LCD_HEIGHT - 2 - ii * 14, POS_L, C_FILL, "%s",
                  d->data.payload);
    ii++;
  }
}

void MESSENGER_deinit() {}

static App meta = {
    .id = APP_MESSENGER,
    .name = "Messenger",
    .runnable = true,
    .init = MESSENGER_init,
    .key = MESSENGER_key,
    .render = MESSENGER_render,
    .deinit = MESSENGER_deinit,
    .update = MESSENGER_update,
};

App *MESSENGER_Meta() { return &meta; }
