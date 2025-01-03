#include "reset.h"
#include "../driver/bk1080.h"
#include "../driver/eeprom.h"
#include "../driver/si473x.h"
#include "../driver/st7565.h"
#include "../driver/uart.h"
#include "../external/CMSIS_5/Device/ARM/ARMCM0/Include/ARMCM0.h"
#include "../helper/bands.h"
#include "../helper/channels.h"
#include "../helper/measurements.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../settings.h"
#include "../svc_render.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"

typedef enum {
  RESET_0xFF,
  RESET_FULL,
  RESET_CHANNELS,
  RESET_BANDS,
  RESET_UNKNOWN,
} ResetType;

typedef struct {
  uint32_t bytes;
  uint16_t channels;
  uint8_t bands;
  uint8_t vfos;
  uint8_t settings;
} Stats;

typedef struct {
  uint32_t eepromSize;
  uint32_t bytes;
  uint16_t pageSize;
  uint16_t channels;
  uint16_t mr;
  uint8_t bands;
  uint8_t vfos;
  uint8_t settings;
} Total;

static char *RESET_TYPE_NAMES[] = {
    "0xFF",
    "FULL",
    "CHANNELS",
    "BANDS",
};

static Stats stats;
static Total total;
static ResetType resetType = RESET_UNKNOWN;

static void selectEeprom(EEPROMType t) {
  gSettings.eepromType = t;

  total.eepromSize = SETTINGS_GetEEPROMSize();
  total.pageSize = SETTINGS_GetPageSize();
  total.mr = CHANNELS_GetCountMax();

  total.settings = 1;
  total.vfos = ARRAY_SIZE(gVFO);
  total.bands = BANDS_DefaultCount();
  total.channels = total.mr - total.vfos - total.bands;
}

static void startReset(ResetType t) {
  resetType = t;

  stats.settings = total.settings;
  stats.vfos = total.vfos;
  stats.bands = total.bands;
  stats.channels = total.channels;

  stats.bytes = 0;

  switch (resetType) {
  case RESET_0xFF:
    total.bytes = total.eepromSize;
    return;
  case RESET_BANDS:
    stats.bands = 0;
    break;
  case RESET_CHANNELS:
    stats.channels = 0;
    break;
  case RESET_FULL:
    stats.settings = 0;
    stats.vfos = 0;
    stats.bands = 0;
    stats.channels = 0;
    break;
  default:
    break;
  }
  total.bytes = (total.settings - stats.settings) * SETTINGS_SIZE +
                (total.vfos - stats.vfos) * CH_SIZE +
                (total.bands - stats.bands) * CH_SIZE +
                (total.channels - stats.channels) * CH_SIZE;
}

static bool resetFull() {
  if (stats.settings < total.settings) {
    SETTINGS_Save();
    stats.settings++;
    stats.bytes += SETTINGS_SIZE;
    return false;
  }

  if (stats.vfos < total.vfos) {
    VFO *vfo = &gVFO[stats.vfos];
    vfo->channel = -1;
    vfo->modulation = MOD_FM;
    vfo->bw = BK4819_FILTER_BW_12k;
    vfo->radio = RADIO_BK4819;
    vfo->txF = 0;
    vfo->offsetDir = OFFSET_NONE;
    vfo->allowTx = false;
    vfo->gainIndex = AUTO_GAIN_INDEX;
    vfo->code.rx.type = 0;
    vfo->code.tx.type = 0;
    vfo->meta.readonly = false;
    vfo->misc.lastUsedFreq = vfo->rxF;
    vfo->meta.type = TYPE_VFO;
    vfo->squelch.value = 4;
    vfo->step = STEP_25_0kHz;
    CHANNELS_Save(total.mr - total.vfos + stats.vfos, vfo);
    stats.vfos++;
    stats.bytes += CH_SIZE;
    return false;
  }

  if (stats.bands < total.bands) {
    Band b = BANDS_GetDefaultBand(stats.bands);
    if (!hasSi && b.txF < BK4819_F_MIN) {
      // skip unsupported bands
      stats.bands++;
      stats.bytes += CH_SIZE;
      return false;
    }

    CHANNELS_Save(total.channels + stats.bands, &b);
    stats.bands++;
    stats.bytes += CH_SIZE;
    return false;
  }

  if (stats.channels < total.channels) {
    CHANNELS_Delete(stats.channels);
    stats.channels++;
    stats.bytes += CH_SIZE;
    return false;
  }
  return true;
}

static bool unreborn(void) {
  const uint16_t PAGE = stats.bytes / total.pageSize;
  EEPROM_ClearPage(PAGE);
  stats.bytes += total.pageSize;
  return stats.bytes >= total.bytes;
}

void RESET_Init(void) {
  resetType = RESET_UNKNOWN;
  gSettings.eepromType = EEPROM_UNKNOWN;
  gSettings.keylock = false;
}

void RESET_Update(void) {
  if (gSettings.eepromType == EEPROM_UNKNOWN || resetType == RESET_UNKNOWN) {
    return;
  }

  if (Now() - gLastRender > 150) {
    gRedrawScreen = true;
  }

  bool status = true;

  switch (resetType) {
  case RESET_0xFF:
    status = unreborn();
    break;
  case RESET_BANDS:
  case RESET_CHANNELS:
  case RESET_FULL:
    status = resetFull();
    break;
  default:
    break;
  }
  if (!status) {
    return;
  }

  NVIC_SystemReset();
}

void RESET_Render(void) {
  if (gSettings.eepromType == EEPROM_UNKNOWN) {
    for (uint8_t i = 0; i < ARRAY_SIZE(EEPROM_TYPE_NAMES); ++i) {
      PrintMedium(2, 18 + i * 8, "%u: %s", i + 1, EEPROM_TYPE_NAMES[i]);
    }
    return;
  }

  if (resetType == RESET_UNKNOWN) {
    for (uint8_t i = 0; i < ARRAY_SIZE(RESET_TYPE_NAMES); ++i) {
      PrintMedium(2, 18 + i * 8, "%u: %s", i, RESET_TYPE_NAMES[i]);
    }
    return;
  }

  STATUSLINE_SetText("%s: %s", EEPROM_TYPE_NAMES[gSettings.eepromType],
                     RESET_TYPE_NAMES[resetType]);

  uint8_t progress = ConvertDomain(stats.bytes, 0, total.bytes, 0, 100);

  const uint8_t TOP = 28;

  DrawRect(13, TOP, 102, 9, C_FILL);
  FillRect(14, TOP + 1, progress, 7, C_FILL);
  PrintMediumEx(LCD_XCENTER, TOP + 7, POS_C, C_INVERT, "%u%", progress);
}

bool RESET_key(KEY_Code_t k, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed && !bKeyHeld) {
    if (gSettings.eepromType == EEPROM_UNKNOWN) {
      if (k > KEY_0) {
        uint8_t t = k - 1;
        if (t < ARRAY_SIZE(EEPROM_TYPE_NAMES)) {
          selectEeprom(t);
          return true;
        }
      }
      return false;
    }
    if (resetType == RESET_UNKNOWN) {
      uint8_t t = k - KEY_0;
      if (t < ARRAY_SIZE(RESET_TYPE_NAMES)) {
        startReset(t);
        return true;
      }
      return false;
    }
  }
  return false;
}
