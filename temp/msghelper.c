#include "msghelper.h"
#include "../driver/audio.h"
#include "../driver/system.h"
#include "../external/printf/printf.h"
#include "../helper/presetlist.h"
#include "../misc.h"
#include <string.h>

DataPacket packet;

static DataPacket gMsgBuf[6] = {0};

uint16_t gFSKWriteIndex = 0;

static const bool recvFsk = true;
static const uint16_t TONE2_FREQS[3] = {4646, 7227, 12389};
static uint16_t BW_DEVIATIONS[3] = {1300, 1200, 850};

static const ModemModulation mod = MOD_AFSK_1200;
static MsgStatus msgStatus = READY;

static void addMsg(DataPacket *msg) {
  for (uint8_t i = ARRAY_SIZE(gMsgBuf); i > 0; --i) {
    memcpy(gMsgBuf[i].buf, gMsgBuf[i - 1].buf, sizeof(packet.buf));
  }
  memcpy(gMsgBuf[0].buf, msg->buf, sizeof(packet.buf));
}

DataPacket *MSG_GetMessage(uint8_t i) { return &gMsgBuf[i]; }

void MSG_ConfigureFSK(bool rx) {
  // tone2, + tone2 gain
  BK4819_WriteRegister(BK4819_REG_70, (1u << 7) | (96u << 0));
  BK4819_WriteRegister(BK4819_REG_72, TONE2_FREQS[mod]);

  switch (mod) {
  case MOD_FSK_700:
  case MOD_FSK_450:
    BK4819_WriteRegister(BK4819_REG_58,
                         (3u << 8) |     // 0 FSK RX gain
                             (1u << 0)); // 1 FSK enable
    break;
  case MOD_AFSK_1200:
    BK4819_WriteRegister(BK4819_REG_58,
                         (1u << 13) |     // 1 FSK TX mode selection
                             (7u << 10) | // 0 FSK RX mode selection
                             (3u << 8) |  // 0 FSK RX gain
                             (1u << 1) |  // 1 FSK RX bandwidth setting
                             (1u << 0));  // 1 FSK enable
    break;
  }

  // REG_5A .. bytes 0 & 1 sync pattern
  //
  // <15:8> sync byte 0
  // < 7:0> sync byte 1
  BK4819_WriteRegister(BK4819_REG_5A, 0x3072);

  // REG_5B .. bytes 2 & 3 sync pattern
  //
  // <15:8> sync byte 2
  // < 7:0> sync byte 3
  BK4819_WriteRegister(BK4819_REG_5B, 0x576C);

  // disable CRC
  BK4819_WriteRegister(BK4819_REG_5C, 0x5625);

  // set the almost full threshold
  if (rx) {
    BK4819_WriteRegister(BK4819_REG_5E,
                         (64u << 3) | (1u << 0)); // 0 ~ 127, 0 ~ 7
  }

  // packet size .. sync + packet - size of a single packet

  uint16_t size = sizeof(packet.buf);
  // size -= (fsk_reg59 & (1u << 3)) ? 4 : 2;
  if (rx)
    size = (((size + 1) / 2) * 2) +
           2; // round up to even, else FSK RX doesn't work

  BK4819_WriteRegister(BK4819_REG_5D, (size << 8));

  // clear FIFO's
  BK4819_FskClearFifo();

  // configure main FSK params
  BK4819_WriteRegister(
      BK4819_REG_59,
      ((rx ? 0u : 15u) << 4) | // 0 ~ 15  preamble length .. bit toggling
          (1u << 3)            // 0/1     sync length
  );

  // clear interupts
  BK4819_WriteRegister(BK4819_REG_02, 0);
}

void MSG_ClearPacketBuffer(void) { memset(packet.buf, 0, sizeof(packet.buf)); }

void MSG_FSKSendData(void) {
  // turn off CTCSS/CDCSS during FFSK
  const uint16_t css_val = BK4819_ReadRegister(BK4819_REG_51);
  BK4819_WriteRegister(BK4819_REG_51, 0);

  // set the FM deviation level
  const uint16_t dev_val = BK4819_ReadRegister(BK4819_REG_40);

  BK4819_WriteRegister(BK4819_REG_40,
                       (dev_val & 0xf000) |
                           (BW_DEVIATIONS[gCurrentPreset.bw] & 0xfff));

  const uint16_t filt_val = BK4819_ReadRegister(BK4819_REG_2B);
  BK4819_WriteRegister(BK4819_REG_2B, (1u << 2) | (1u << 0));

  MSG_ConfigureFSK(false);

  SYSTEM_DelayMs(100);

  { // load the entire packet data into the TX FIFO buffer
    for (size_t i = 0, j = 0; i < sizeof(packet.buf); i += 2, j++) {
      BK4819_WriteRegister(BK4819_REG_5F,
                           (packet.buf[i + 1] << 8) | packet.buf[i]);
    }
  }

  // enable FSK TX
  BK4819_FskEnableTx();

  {
    // allow up to 310ms for the TX to complete
    // if it takes any longer then somethings gone wrong, we shut the TX down
    unsigned int timeout = 1000 / 5;

    while (timeout-- > 0) {
      SYSTEM_DelayMs(5);
      if (BK4819_ReadRegister(BK4819_REG_0C) &
          (1u << 0)) { // we have interrupt flags
        BK4819_WriteRegister(BK4819_REG_02, 0);
        if (BK4819_ReadRegister(BK4819_REG_02) & BK4819_REG_02_FSK_TX_FINISHED)
          timeout = 0; // TX is complete
      }
    }
  }

  SYSTEM_DelayMs(100);

  // disable TX
  MSG_ConfigureFSK(true);

  // restore FM deviation level
  BK4819_WriteRegister(BK4819_REG_40, dev_val);

  // restore TX/RX filtering
  BK4819_WriteRegister(BK4819_REG_2B, filt_val);

  // restore the CTCSS/CDCSS setting
  BK4819_WriteRegister(BK4819_REG_51, css_val);
}

void MSG_EnableRX(const bool enable) {
  if (enable) {
    MSG_ConfigureFSK(true);

    if (recvFsk) {
      BK4819_FskEnableRx();
    }
  } else {
    BK4819_WriteRegister(BK4819_REG_70, 0);
    BK4819_WriteRegister(BK4819_REG_58, 0);
  }
}

void MSG_SendPacket(void) {
  if (msgStatus != READY || strlen((char *)packet.data.payload) == 0) {
    return;
  }

  RADIO_ToggleTX(true);

  if (gTxState != TX_ON) {
    return;
  }

  msgStatus = SENDING;

  SYSTEM_DelayMs(50);

  MSG_FSKSendData();

  SYSTEM_DelayMs(50);

  RADIO_ToggleTX(false);

  MSG_EnableRX(true);

  MSG_ClearPacketBuffer();

  msgStatus = READY;
}

void MSG_SendAck(void) {
  MSG_ClearPacketBuffer();
  packet.data.header = ACK_PACKET;
  // sending only empty header seems to not work
  memset(packet.data.payload, 255, 5);
  MSG_SendPacket();
}

void MSG_HandleReceive(void) {
  if (packet.data.header == ACK_PACKET) {
  } else {
    if (packet.data.header >= INVALID_PACKET) {
      // so sad
    } else {
      // handle dataPacket.data.payload
      addMsg(&packet);
    }
  }

  // Transmit a message to the sender that we have received the message
  if (packet.data.header == MESSAGE_PACKET) {
    // wait so the correspondent radio can properly receive it
    SYSTEM_DelayMs(700);
    MSG_SendAck();
  }
}

void MSG_StorePacket(const uint16_t intBits) {
  const bool rxSync = intBits & BK4819_REG_02_FSK_RX_SYNC;
  const bool rxFifoAlmostFull = intBits & BK4819_REG_02_FSK_FIFO_ALMOST_FULL;
  const bool rxFinished = intBits & BK4819_REG_02_FSK_RX_FINISHED;

  if (rxSync) {
    AUDIO_ToggleSpeaker(false);
    MSG_ClearPacketBuffer();
    gFSKWriteIndex = 0;
    msgStatus = RECEIVING;
  }

  if (rxFifoAlmostFull && msgStatus == RECEIVING) {

    const uint16_t count =
        BK4819_ReadRegister(BK4819_REG_5E) & (7u << 0); // almost full threshold
    for (uint16_t i = 0; i < count; i++) {
      const uint16_t word = BK4819_ReadRegister(BK4819_REG_5F);
      if (gFSKWriteIndex < sizeof(packet.buf))
        packet.buf[gFSKWriteIndex++] = (word >> 0) & 0xff;
      if (gFSKWriteIndex < sizeof(packet.buf))
        packet.buf[gFSKWriteIndex++] = (word >> 8) & 0xff;
    }

    SYSTEM_DelayMs(10);
  }

  if (rxFinished) {
    BK4819_FskClearFifo();
    BK4819_FskEnableRx();
    msgStatus = READY;

    if (gFSKWriteIndex > 2) {
      MSG_HandleReceive();
    }
    gFSKWriteIndex = 0;
  }
}

void MSG_Send(const char *message) {
  MSG_ClearPacketBuffer();
  packet.data.header = MESSAGE_PACKET;
  // snprintf(packet.data.nick, 10, "%s", gSettings.nickName);
  snprintf(packet.data.payload, 30, "%s", message);
  MSG_SendPacket();
}

void MSG_Init(void) { msgStatus = READY; }
