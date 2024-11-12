#include "menu.h"
#include "../dcs.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
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
  PrintMedium(4, by, "%s", name);
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

void PrintRTXCode(char *Output, uint8_t codeType, uint8_t code) {
  if (codeType) {
    if (codeType == CODE_TYPE_CONTINUOUS_TONE) {
      sprintf(Output, "CT:%u.%uHz", CTCSS_Options[code] / 10,
              CTCSS_Options[code] % 10);
    } else if (codeType == CODE_TYPE_DIGITAL) {
      sprintf(Output, "DCS:D%03oN", DCS_Options[code]);
    } else if (codeType == CODE_TYPE_REVERSE_DIGITAL) {
      sprintf(Output, "DCS:D%03oI", DCS_Options[code]);
    } else {
      sprintf(Output, "No code");
    }
  }
}

void GetMenuItemValue(PresetCfgMenu type, char *Output) {
  Band *band = gCurrentPreset;
  uint32_t fs = band->rxF;
  uint32_t fe = band->txF;
  bool isVfo = gCurrentApp == APP_VFO_CFG;
  switch (type) {
  case M_RADIO:
    strncpy(Output, radioNames[isVfo ? radio->radio : gCurrentPreset->radio],
            31);
    break;
  case M_START:
    sprintf(Output, "%lu.%03lu", fs / MHZ, fs / 100 % 1000);
    break;
  case M_END:
    sprintf(Output, "%lu.%03lu", fe / MHZ, fe / 100 % 1000);
    break;
  case M_NAME:
    strncpy(Output, band->name, 31);
    break;
  case M_BW:
    strncpy(Output, RADIO_GetBWName(band->bw), 31);
    break;
  case M_SQ_TYPE:
    strncpy(Output, sqTypeNames[band->squelch.type], 31);
    break;
  case M_SQ:
    sprintf(Output, "%u", band->squelch);
    break;
  case M_GAIN:
    sprintf(Output, "%ddB", -gainTable[band->gainIndex].gainDb + 33);
    break;
  case M_MODULATION:
    strncpy(Output,
            modulationTypeOptions[isVfo ? radio->modulation
                                        : gCurrentPreset->modulation],
            31);
    break;
  case M_STEP:
    sprintf(Output, "%u.%02uKHz", StepFrequencyTable[band->step] / 100,
            StepFrequencyTable[band->step] % 100);
    break;
  case M_TX:
    strncpy(Output, yesNo[gCurrentPreset->allowTx], 31);
    break;
  case M_F_RX:
    sprintf(Output, "%u.%05u", radio->rxF / MHZ, radio->rxF % MHZ);
    break;
  case M_F_TX:
    sprintf(Output, "%u.%05u", radio->txF / MHZ, radio->txF % MHZ);
    break;
  case M_RX_CODE_TYPE:
    strncpy(Output, TX_CODE_TYPES[radio->code.rx.type], 31);
    break;
  case M_RX_CODE:
    PrintRTXCode(Output, radio->code.rx.type, radio->code.rx.value);
    break;
  case M_TX_CODE_TYPE:
    strncpy(Output, TX_CODE_TYPES[radio->code.tx.type], 31);
    break;
  case M_TX_CODE:
    PrintRTXCode(Output, radio->code.tx.type, radio->code.tx.value);
    break;
  case M_TX_OFFSET:
    sprintf(Output, "%u.%05u", radio->txF / MHZ, gCurrentPreset->txF % MHZ);
    break;
  case M_TX_OFFSET_DIR:
    snprintf(Output, 15, TX_OFFSET_NAMES[radio->offsetDir]);
    break;
  case M_F_TXP:
    snprintf(Output, 15, TX_POWER_NAMES[gCurrentPreset->power]);
    break;
  default:
    break;
  }
}

void AcceptRadioConfig(const MenuItem *item, uint8_t subMenuIndex) {
  bool isVfo = gCurrentApp == APP_VFO_CFG;
  switch (item->type) {
  case M_BW:
    gCurrentPreset->bw = subMenuIndex;
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
    if (isVfo) {
      radio->modulation = subMenuIndex;
      RADIO_SaveCurrentVFO();
      RADIO_SetupByCurrentVFO();
    } else {
      gCurrentPreset->modulation = subMenuIndex;
      PRESETS_SaveCurrent();
    }
    break;
  case M_STEP:
    gCurrentPreset->step = subMenuIndex;
    PRESETS_SaveCurrent();
    break;
  case M_SQ_TYPE:
    gCurrentPreset->squelch.type = subMenuIndex;
    BK4819_SquelchType(subMenuIndex);
    PRESETS_SaveCurrent();
    break;
  case M_SQ:
    gCurrentPreset->squelch.value = subMenuIndex;
    BK4819_Squelch(subMenuIndex, radio->rxF, gSettings.sqlOpenTime,
                   gSettings.sqlCloseTime);
    PRESETS_SaveCurrent();
    break;

  case M_GAIN:
    gCurrentPreset->gainIndex = subMenuIndex;
    BK4819_SetAGC(RADIO_GetModulation() != MOD_AM, gCurrentPreset->gainIndex);
    PRESETS_SaveCurrent();
    break;
  case M_TX:
    gCurrentPreset->allowTx = subMenuIndex;
    PRESETS_SaveCurrent();
    break;
  case M_RX_CODE_TYPE:
    radio->code.rx.type = subMenuIndex;
    RADIO_SaveCurrentVFO();
    break;
  case M_RX_CODE:
    radio->code.rx.value = subMenuIndex;
    RADIO_SaveCurrentVFO();
    break;
  case M_TX_CODE_TYPE:
    radio->code.tx.type = subMenuIndex;
    RADIO_SaveCurrentVFO();
    break;
  case M_TX_CODE:
    radio->code.tx.value = subMenuIndex;
    RADIO_SaveCurrentVFO();
    break;
  case M_RADIO:
    if (isVfo) {
      radio->radio = subMenuIndex;
      RADIO_SaveCurrentVFO();
    } else {
      gCurrentPreset->radio = subMenuIndex;
      PRESETS_SaveCurrent();
    }
    break;

  default:
    break;
  }
}

void OnRadioSubmenuChange(const MenuItem *item, uint8_t subMenuIndex) {
  switch (item->type) {
  case M_GAIN:
    RADIO_SetGain(subMenuIndex);
    break;
  case M_BW:
    gCurrentPreset->bw = subMenuIndex;
    RADIO_SetupBandParams();
    break;
  default:
    break;
  }
}
