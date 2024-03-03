#include "menu.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../radio.h"
#include "graphics.h"

const char *onOff[] = {"Off", "On"};
const char *yesNo[] = {"No", "Yes"};

void UI_DrawScrollBar(const uint16_t size, const uint16_t iCurrent,
                      const uint8_t nLines) {
  const uint8_t sbY =
      ConvertDomain(iCurrent, 0, size, 0, nLines * MENU_ITEM_H - 3);

  DrawVLine(LCD_WIDTH - 2, MENU_Y, LCD_HEIGHT - MENU_Y, C_FILL);

  FillRect(LCD_WIDTH - 3, MENU_Y + sbY, 3, 3, C_FILL);
}

void UI_ShowMenuItem(uint8_t line, const char *name, bool isCurrent) {
  uint8_t by = MENU_Y + line * MENU_ITEM_H + 8;
  PrintMedium(4, by, name);
  if (isCurrent) {
    FillRect(0, MENU_Y + line * MENU_ITEM_H, LCD_WIDTH - 3, MENU_ITEM_H,
             C_INVERT);
  }
}

void UI_ShowMenuSimple(const MenuItem *menu, uint16_t size,
                       uint16_t currentIndex) {
  const uint16_t maxItems =
      size < MENU_LINES_TO_SHOW ? size : MENU_LINES_TO_SHOW;
  const uint16_t offset = Clamp(currentIndex - 2, 0, size - maxItems);
  char name[32] = "";

  for (uint16_t i = 0; i < maxItems; ++i) {
    uint16_t itemIndex = i + offset;
    strncpy(name, menu[itemIndex].name, 31);
    PrintSmallEx(LCD_WIDTH - 4, MENU_Y + i * MENU_ITEM_H + 8, POS_R, C_FILL,
                 "%u", itemIndex + 1);
    UI_ShowMenuItem(i, name, currentIndex == itemIndex);
  }

  UI_DrawScrollBar(size, currentIndex, MENU_LINES_TO_SHOW);
}

void UI_ShowMenu(void (*getItemText)(uint16_t index, char *name), uint16_t size,
                 uint16_t currentIndex) {
  const uint16_t maxItems =
      size < MENU_LINES_TO_SHOW ? size : MENU_LINES_TO_SHOW;
  const uint16_t offset = Clamp(currentIndex - 2, 0, size - maxItems);

  for (uint16_t i = 0; i < maxItems; ++i) {
    char name[32] = "";
    uint16_t itemIndex = i + offset;
    getItemText(itemIndex, name);
    PrintSmallEx(LCD_WIDTH - 4, MENU_Y + i * MENU_ITEM_H + 8, POS_R, C_FILL,
                 "%u", itemIndex + 1);
    UI_ShowMenuItem(i, name, currentIndex == itemIndex);
  }

  UI_DrawScrollBar(size, currentIndex, MENU_LINES_TO_SHOW);
}

void UI_ShowMenuEx(void (*showItem)(uint16_t i, uint16_t index, bool isCurrent),
                   uint16_t size, uint16_t currentIndex, uint16_t linesMax) {
  const uint16_t maxItems = size < linesMax ? size : linesMax;
  const uint16_t offset = Clamp(currentIndex - 2, 0, size - maxItems);

  for (uint16_t i = 0; i < maxItems; ++i) {
    uint16_t itemIndex = i + offset;
    showItem(i, itemIndex, currentIndex == itemIndex);
  }

  UI_DrawScrollBar(size, currentIndex, linesMax);
}

#include "../helper/presetlist.h"

void GetMenuItemValue(PresetCfgMenu type, char *Output) {
  Band *band = &gCurrentPreset->band;
  uint32_t fs = band->bounds.start;
  uint32_t fe = band->bounds.end;
  switch (type) {
  case M_START:
    sprintf(Output, "%lu.%03lu", fs / 100000, fs / 100 % 1000);
    break;
  case M_END:
    sprintf(Output, "%lu.%03lu", fe / 100000, fe / 100 % 1000);
    break;
  case M_NAME:
    strncpy(Output, band->name, 31);
    break;
  case M_BW:
    strncpy(Output, bwNames[band->bw], 31);
    break;
  case M_SQ_TYPE:
    strncpy(Output, sqTypeNames[band->squelchType], 31);
    break;
  case M_SQ:
    sprintf(Output, "%u", band->squelch);
    break;
  case M_GAIN:
    sprintf(Output, "%ddB", gainTable[band->gainIndex].gainDb);
    break;
  case M_MODULATION:
    strncpy(Output, modulationTypeOptions[band->modulation], 31);
    break;
  case M_STEP:
    sprintf(Output, "%u.%02uKHz", StepFrequencyTable[band->step] / 100,
            StepFrequencyTable[band->step] % 100);
    break;
  case M_TX:
    strncpy(Output, yesNo[gCurrentPreset->allowTx], 31);
    break;
  case M_F_RX:
    sprintf(Output, "%u.%05u", radio->rx.f / 100000, radio->rx.f % 100000);
    break;
  case M_F_TX:
    sprintf(Output, "%u.%05u", radio->tx.f / 100000, radio->tx.f % 100000);
    break;
  case M_TX_OFFSET:
    sprintf(Output, "%u.%05u", gCurrentPreset->offset / 100000,
            gCurrentPreset->offset % 100000);
    break;
  case M_TX_OFFSET_DIR:
    snprintf(Output, 15, TX_OFFSET_NAMES[gCurrentPreset->offsetDir]);
    break;
  case M_F_TXP:
    snprintf(Output, 15, TX_POWER_NAMES[gCurrentPreset->power]);
    break;
  default:
    break;
  }
}

void AcceptRadioConfig(const MenuItem *item, uint8_t subMenuIndex) {
  switch (item->type) {
  case M_BW:
    gCurrentPreset->band.bw = subMenuIndex;
    BK4819_SetFilterBandwidth(subMenuIndex);
    PRESETS_SaveCurrent();
    break;
  case M_F_TXP:
    gCurrentPreset->power = subMenuIndex;
    PRESETS_SaveCurrent();
    break;
  case M_TX_OFFSET_DIR:
    gCurrentPreset->offsetDir = subMenuIndex;
    PRESETS_SaveCurrent();
    break;
  case M_MODULATION:
    gCurrentPreset->band.modulation = subMenuIndex;
    BK4819_SetModulation(subMenuIndex);
    PRESETS_SaveCurrent();
    break;
  case M_STEP:
    gCurrentPreset->band.step = subMenuIndex;
    PRESETS_SaveCurrent();
    break;
  case M_SQ_TYPE:
    gCurrentPreset->band.squelchType = subMenuIndex;
    BK4819_SquelchType(subMenuIndex);
    PRESETS_SaveCurrent();
    break;
  case M_SQ:
    gCurrentPreset->band.squelch = subMenuIndex;
    BK4819_Squelch(subMenuIndex, radio->rx.f, radio->sq.openTime,
                   radio->sq.closeTime);
    PRESETS_SaveCurrent();
    break;

  case M_GAIN:
    gCurrentPreset->band.gainIndex = subMenuIndex;
    BK4819_SetGain(subMenuIndex);
    PRESETS_SaveCurrent();
    break;
  case M_TX:
    gCurrentPreset->allowTx = subMenuIndex;
    PRESETS_SaveCurrent();
    break;

  default:
    break;
  }
}
