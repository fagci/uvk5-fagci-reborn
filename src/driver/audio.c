#include "audio.h"
#include "../inc/dp32g030/gpio.h"
#include "bk4819.h"
#include "gpio.h"
#include "system.h"

static bool speakerOn = false;

void AUDIO_ToggleSpeaker(bool on) {
  speakerOn = on;
  if (on) {
    GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
  } else {
    GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);
  }
}

void AUDIO_PlayTone(uint32_t frequency, uint16_t duration) {
  bool isSpeakerWasOn = speakerOn;
  uint16_t ToneConfig = BK4819_ReadRegister(BK4819_REG_71);

  AUDIO_ToggleSpeaker(false);

  SYSTEM_DelayMs(10);
  BK4819_PlayTone(frequency, true);
  SYSTEM_DelayMs(2);

  AUDIO_ToggleSpeaker(true);
  SYSTEM_DelayMs(60);

  BK4819_ExitTxMute();
  SYSTEM_DelayMs(duration);
  BK4819_EnterTxMute();

  SYSTEM_DelayMs(10);
  AUDIO_ToggleSpeaker(false);

  SYSTEM_DelayMs(5);
  BK4819_TurnsOffTones_TurnsOnRX();
  SYSTEM_DelayMs(5);

  BK4819_WriteRegister(BK4819_REG_71, ToneConfig);
  AUDIO_ToggleSpeaker(isSpeakerWasOn);
}
