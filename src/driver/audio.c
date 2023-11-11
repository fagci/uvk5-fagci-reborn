/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include "audio.h"
#include "driver/bk4819.h"
#include "driver/gpio.h"
#include "driver/system.h"
#include "driver/systick.h"
#include "inc/dp32g030/gpio.h"
#include "misc.h"

bool speakerOn = false;

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
  // BK4819_RX_TurnOn();

  SYSTEM_DelayMs(20);
  BK4819_PlayTone(frequency, true);
  SYSTEM_DelayMs(2);

  AUDIO_ToggleSpeaker(true);
  SYSTEM_DelayMs(60);

  BK4819_ExitTxMute();
  SYSTEM_DelayMs(duration);
  BK4819_EnterTxMute();

  SYSTEM_DelayMs(20);
  AUDIO_ToggleSpeaker(false);

  SYSTEM_DelayMs(5);
  BK4819_TurnsOffTones_TurnsOnRX();
  SYSTEM_DelayMs(5);

  BK4819_WriteRegister(BK4819_REG_71, ToneConfig);
  AUDIO_ToggleSpeaker(isSpeakerWasOn);
}
