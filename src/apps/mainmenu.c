#include "mainmenu.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../misc.h"
#include "../radio.h"
#include "../ui/helper.h"
#include "apps.h"

typedef enum {
  M_NONE,
  M_UPCONVERTER,
  M_SPECTRUM,
  M_STILL,
  M_TASK_MANAGER,
  M_RESET,
} Menu;

typedef enum {
  MT_ITEMS,
  MT_RANGE,
  MT_INPUT,
  MT_RUN,
} MenuItemType;

typedef struct MenuItem {
  const char *name;
  Menu type;
  uint8_t size;
} MenuItem;

#define ITEMS(value)                                                           \
  items = value;                                                               \
  size = ARRAY_SIZE(value);                                                    \
  type = MT_ITEMS;

static const uint8_t LINES_TO_SHOW = 6;

static uint8_t menuIndex = 0;
static uint8_t subMenuIndex = 0;
static bool isSubMenu = false;

static const MenuItem menu[] = {
    {"Upconverter", M_UPCONVERTER, 3},
    {"Spectrum", M_SPECTRUM},
    {"Still", M_STILL},
    {"Task manager", M_TASK_MANAGER},
    {"EEPROM reset", M_RESET},
    {"Test 4"},
    {"Test 5"},
    {"Test 6"},
};

void accept() {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_UPCONVERTER: {
    uint32_t f = GetScreenF(gCurrentVfo.fRX);
    gUpconverterType = subMenuIndex;
    RADIO_TuneTo(GetTuneF(f), true);
    isSubMenu = false;
  }; break;
  default:
    break;
  }
}

static const char *getValue(Menu type) {
  switch (type) {
  case M_UPCONVERTER:
    return upConverterFreqNames[gUpconverterType];
  default:
    break;
  }
  return "";
}

static void ShowItem(uint8_t line, const char *name, bool isCurrent) {
  if (isCurrent) {
    gFrameBuffer[line][0] = 0b01111111;
    gFrameBuffer[line][1] = 0b00111110;
    gFrameBuffer[line][2] = 0b00011100;
    gFrameBuffer[line][3] = 0b00001000;
    UI_PrintStringSmallBold(name, 6, 6, line);
  } else {
    UI_PrintStringSmall(name, 6, 6, line);
  }
}

static void DrawScrollBar(const uint8_t size, const uint8_t currentIndex,
                          const uint8_t linesCount) {
  uint8_t i;
  const uint8_t scrollbarPosY =
      ConvertDomain(currentIndex, 0, size, 0, linesCount * 8 + 5);

  for (i = 0; i < linesCount; i++) {
    gFrameBuffer[i][126] = 0xFF;
  }

  for (i = 0; i < 3; i++) {
    PutPixel(127, scrollbarPosY + i, true);
    PutPixel(125, scrollbarPosY + i, true);
  }
}

void showMenu(const MenuItem *items, uint8_t size, uint8_t currentIndex) {

  const uint8_t maxItems = size < LINES_TO_SHOW ? size : LINES_TO_SHOW;
  const uint8_t offset = Clamp(currentIndex - 2, 0, size - maxItems);

  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

  for (uint8_t i = 0; i < maxItems; ++i) {
    uint8_t itemIndex = i + offset;
    const MenuItem *item = &items[itemIndex];
    ShowItem(i, item->name, currentIndex == itemIndex);
  }

  DrawScrollBar(size, currentIndex, LINES_TO_SHOW);
}

void showItems(const char **items, uint8_t size, uint8_t currentIndex) {
  const uint8_t maxItems = size < LINES_TO_SHOW ? size : LINES_TO_SHOW;
  const uint8_t offset = Clamp(currentIndex - 2, 0, size - maxItems);

  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

  for (uint8_t i = 0; i < maxItems; ++i) {
    uint8_t itemIndex = i + offset;
    ShowItem(i, items[itemIndex], currentIndex == itemIndex);
  }

  DrawScrollBar(size, currentIndex, LINES_TO_SHOW);
}

void showSubmenu(Menu menu) {
  const char **items;
  uint8_t size;
  MenuItemType type;

  switch (menu) {
  case M_UPCONVERTER:
    ITEMS(upConverterFreqNames);
    break;
  default:
    break;
  }

  switch (type) {
  case MT_ITEMS:
    showItems(items, size, subMenuIndex);
    break;
  default:
    break;
  }
}

void MAINMENU_render() {
  memset(gStatusLine, 0, sizeof(gStatusLine));
  const MenuItem *item = &menu[menuIndex];
  if (isSubMenu) {
    showSubmenu(item->type);
    UI_PrintStringSmallest(item->name, 0, 0, true, true);
  } else {
    showMenu(menu, ARRAY_SIZE(menu), menuIndex);
    UI_PrintStringSmall(getValue(item->type), 1, 126, 6);
  }
}

void MAINMENU_init() {
  isSubMenu = false;
}
void MAINMENU_update() {}
void MAINMENU_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed || bKeyHeld) {
    return;
  }
  const MenuItem *item = &menu[menuIndex];
  const uint8_t MENU_SIZE = ARRAY_SIZE(menu);
  const uint8_t SUBMENU_SIZE = item->size;
  switch (key) {
  case KEY_UP:
    if (isSubMenu) {
      subMenuIndex = subMenuIndex == 0 ? SUBMENU_SIZE - 1 : subMenuIndex - 1;
    } else {
      menuIndex = menuIndex == 0 ? MENU_SIZE - 1 : menuIndex - 1;
    }
    gRedrawScreen = true;
    break;
  case KEY_DOWN:
    if (isSubMenu) {
      subMenuIndex = subMenuIndex == SUBMENU_SIZE - 1 ? 0 : subMenuIndex + 1;
    } else {
      menuIndex = menuIndex == MENU_SIZE - 1 ? 0 : menuIndex + 1;
    }
    gRedrawScreen = true;
    break;
  case KEY_MENU:
    // RUN APPS HERE
    switch (item->type) {
    case M_SPECTRUM:
      APPS_run(APP_SPECTRUM);
      break;
    case M_STILL:
      APPS_run(APP_STILL);
      break;
    case M_TASK_MANAGER:
      APPS_run(APP_TASK_MANAGER);
      break;
    case M_RESET:
      APPS_run(APP_RESET);
      break;
    default:
      break;
    }
    if (isSubMenu) {
      accept();
    } else {
      isSubMenu = true;
    }
    gRedrawStatus = true;
    gRedrawScreen = true;
    break;
  case KEY_EXIT:
    if (isSubMenu) {
      isSubMenu = false;
    } else {
      APPS_run(gPreviousApp);
    }
    gRedrawScreen = true;
    gRedrawStatus = true;
    break;
  default:
    break;
  }
}
