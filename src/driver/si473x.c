#include "si473x.h"
#include "../inc/dp32g030/gpio.h"
#include "audio.h"
#include "gpio.h"
#include "i2c.h"
#include "system.h"
#include "systick.h"

#define SI47XX_DELAY_MS 400

#define RST_HIGH GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_BK1080)
#define RST_LOW GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_BK1080)

// Command arguments

RSQStatus rsqStatus;

SI47XX_MODE si4732mode = SI47XX_FM;
uint16_t siCurrentFreq = 10320;

static const uint8_t SI47XX_I2C_ADDR = 0x22;

void SI47XX_ReadBuffer(uint8_t *buf, uint8_t size) {
  I2C_Start();
  I2C_Write(SI47XX_I2C_ADDR + 1);
  I2C_ReadBuffer(buf, size);
  I2C_Stop();
}

void SI47XX_WriteBuffer(uint8_t *buf, uint8_t size) {
  I2C_Start();
  I2C_Write(SI47XX_I2C_ADDR);
  I2C_WriteBuffer(buf, size);
  I2C_Stop();
}

void waitToSend() {
  uint8_t tmp = 0;
  SI47XX_ReadBuffer((uint8_t *)&tmp, 1);
  while (!(tmp & STATUS_CTS)) {
    SYSTICK_DelayUs(100);
    SI47XX_ReadBuffer((uint8_t *)&tmp, 1);
  }
}

void sendProperty(uint16_t prop, uint16_t parameter) {
  waitToSend();
  uint8_t tmp[6] = {CMD_SET_PROPERTY, 0, prop >> 8, prop & 0xff, parameter >> 8,
                    parameter & 0xff};
  SI47XX_WriteBuffer(tmp, 6);
  SYSTEM_DelayMs(10); // irrespective of CTS coming up earlier than that
}

uint16_t getProperty(uint16_t prop, bool *valid) {
  uint8_t response[4] = {0};
  uint8_t tmp[4] = {CMD_GET_PROPERTY, 0, prop >> 8, prop & 0xff};
  waitToSend();
  SI47XX_WriteBuffer(tmp, 4);
  SI47XX_ReadBuffer(response, 4);

  if (valid) {
    *valid = !(response[0] & STATUS_ERR);
  }

  return (response[2] << 8) | response[3];
}

void RSQ_GET() {
  uint8_t cmd[2] = {CMD_FM_RSQ_STATUS, 0x01};
  if (si4732mode == SI47XX_AM) {
    cmd[0] = CMD_AM_RSQ_STATUS;
  }

  waitToSend();
  SI47XX_WriteBuffer(cmd, 2);
  SI47XX_ReadBuffer(rsqStatus.raw, si4732mode == SI47XX_FM ? 8 : 6);
}

void setVolume(uint8_t volume) {
  if (volume < 0)
    volume = 0;
  if (volume > 63)
    volume = 63;
  sendProperty(PROP_RX_VOLUME, volume);
}

void setAvcAmMaxGain(uint8_t gain) {
  if (gain < 12 || gain > 90)
    return;
  sendProperty(PROP_AM_AUTOMATIC_VOLUME_CONTROL_MAX_GAIN, gain * 340);
}

void enableRDS(void) {
  // Enable and configure RDS reception
  if (si4732mode == SI47XX_FM) {
    sendProperty(PROP_FM_RDS_INT_SOURCE, FLG_RDSRECV);
    // Set the FIFO high-watermark to 12 RDS blocks, which is safe even for
    // old chips, yet large enough to improve performance.
    sendProperty(PROP_FM_RDS_INT_FIFO_COUNT, 12);
    sendProperty(
        PROP_FM_RDS_CONFIG,
        ((FLG_BLETHA_35 | FLG_BLETHB_35 | FLG_BLETHC_35 | FLG_BLETHD_35) << 8) |
            FLG_RDSEN);
  };
}

void SI47XX_SetAutomaticGainControl(uint8_t AGCDIS, uint8_t AGCIDX) {
  SI47XX_AgcOverrride agc;

  uint8_t cmd;

  if (si4732mode == SI47XX_FM)
    cmd = CMD_FM_AGC_OVERRIDE;
  else
    cmd = CMD_AM_AGC_OVERRIDE;

  agc.arg.DUMMY = 0; // ARG1: bits 7:1 Always write to 0;
  agc.arg.AGCDIS = AGCDIS;
  agc.arg.AGCIDX = AGCIDX;

  waitToSend();

  uint8_t cmd2[] = {cmd, agc.raw[0], agc.raw[1]};
  SI47XX_WriteBuffer(cmd2, 3);
}

void SI47XX_PowerUp() {
  RST_HIGH;
  // SYSTEM_DelayMs(10);
  uint8_t cmd[3] = {CMD_POWER_UP, FLG_XOSCEN | FUNC_FM, OUT_ANALOG};
  if (si4732mode == SI47XX_AM) {
    cmd[1] = FLG_XOSCEN | FUNC_AM;
  }
  waitToSend();
  SI47XX_WriteBuffer(cmd, 3);
  SYSTEM_DelayMs(500);

  waitToSend(); // for stability. gpo... maybe not needed
  uint8_t cmd2[2] = {CMD_GPIO_CTL, FLG_GPO1OEN | FLG_GPO2OEN};

  SI47XX_WriteBuffer(cmd2, 2);
  AUDIO_ToggleSpeaker(true);
  setVolume(63);

  if (si4732mode == SI47XX_FM) {
    enableRDS();
  } else {
    SI47XX_SetAutomaticGainControl(1, 0);
    sendProperty(PROP_AM_SOFT_MUTE_MAX_ATTENUATION, 0);
    sendProperty(PROP_AM_AUTOMATIC_VOLUME_CONTROL_MAX_GAIN, 0x7800);
    SI47XX_SetSeekAmLimits(1800, 30000);
  }
  SI47XX_SetFreq(siCurrentFreq);
}

void SI47XX_Seek(bool up, bool wrap) {
  uint8_t seekOpt = (up ? FLG_SEEKUP : 0) | (wrap ? FLG_WRAP : 0);
  uint8_t cmd[6] = {CMD_FM_SEEK_START, seekOpt, 0x00, 0x00, 0x00, 0x00};

  if (si4732mode == SI47XX_AM) {
    cmd[0] = CMD_AM_SEEK_START;
    cmd[5] = (siCurrentFreq > 1800) ? 1 : 0;
  }

  waitToSend();
  SI47XX_WriteBuffer(cmd, si4732mode == SI47XX_FM ? 2 : 6);
}

uint16_t SI47XX_getFrequency(bool *valid) {
  uint8_t response[4] = {0};
  uint8_t cmd[1] = {CMD_FM_TUNE_STATUS};

  if (si4732mode == SI47XX_AM) {
    cmd[0] = CMD_AM_TUNE_STATUS;
  }

  waitToSend();
  SI47XX_WriteBuffer(cmd, 1);
  SI47XX_ReadBuffer(response, 4);

  if (valid) {
    *valid = (response[1] & STATUS_VALID);
  }

  return (response[2] << 8) | response[3];
}

void SI47XX_PowerDown() {
  AUDIO_ToggleSpeaker(false);
  uint8_t cmd[1] = {CMD_POWER_DOWN};

  waitToSend();
  SI47XX_WriteBuffer(cmd, 1);
  SYSTICK_Delay250ns(10);
  RST_LOW;
}

void SI47XX_SwitchMode(SI47XX_MODE mode) {
  if (si4732mode != mode) {
    SI47XX_PowerDown();
    si4732mode = mode;
    SI47XX_PowerUp();
  }
}

void SI47XX_SetFreq(uint16_t freq) {
  uint8_t hb = (freq >> 8) & 0xFF;
  uint8_t lb = freq & 0xFF;

  uint8_t size = 4;
  uint8_t cmd[5] = {CMD_FM_TUNE_FREQ, 0x00, hb, lb, 0};

  if (si4732mode == SI47XX_AM) {
    cmd[0] = CMD_AM_TUNE_FREQ;
    size = 5;
  }

  waitToSend();
  SI47XX_WriteBuffer(cmd, size);
  siCurrentFreq = freq;

  SYSTEM_DelayMs(30);
  // RSQ_GET();
}

void SI47XX_SetAMFrontendAGC(uint8_t minGainIdx, uint8_t attnBackup) {
  sendProperty(PROP_AM_FRONTEND_AGC_CONTROL, minGainIdx << 8 | attnBackup);
}

void SI47XX_SetBandwidth(SI47XX_FilterBW AMCHFLT, bool AMPLFLT) {
  sendProperty(PROP_AM_CHANNEL_FILTER, AMCHFLT << 8 | AMPLFLT);
}

void SI47XX_ReadRDS(uint8_t buf[13]) {
  uint8_t cmd[2] = {CMD_FM_RDS_STATUS, RDS_STATUS_ARG1_CLEAR_INT};
  waitToSend();
  SI47XX_WriteBuffer(cmd, 2);
  SI47XX_ReadBuffer(buf, 13);
}

void SI47XX_SetSeekFmLimits(uint16_t bottom, uint16_t top) {
  sendProperty(PROP_FM_SEEK_BAND_BOTTOM, bottom);
  sendProperty(PROP_FM_SEEK_BAND_TOP, top);
}

void SI47XX_SetSeekAmLimits(uint16_t bottom, uint16_t top) {
  sendProperty(PROP_AM_SEEK_BAND_BOTTOM, bottom);
  sendProperty(PROP_AM_SEEK_BAND_TOP, top);
}

void SI47XX_SetSeekFmSpacing(uint16_t spacing) {
  sendProperty(PROP_FM_SEEK_FREQ_SPACING, spacing);
}

void SI47XX_SetSeekAmSpacing(uint16_t spacing) {
  sendProperty(PROP_AM_SEEK_FREQ_SPACING, spacing);
}

void SI47XX_SetSeekFmRssiThreshold(uint16_t value) {
  sendProperty(PROP_FM_SEEK_TUNE_RSSI_THRESHOLD, value);
}

void SI47XX_SetSeekAmRssiThreshold(uint16_t value) {
  sendProperty(PROP_AM_SEEK_TUNE_RSSI_THRESHOLD, value);
}
