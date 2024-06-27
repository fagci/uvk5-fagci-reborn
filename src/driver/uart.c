#include "../inc/dp32g030/uart.h"
#include "../board.h"
#include "../external/CMSIS_5/Device/ARM/ARMCM0/Include/ARMCM0.h"
#include "../external/printf/printf.h"
#include "../inc/dp32g030/dma.h"
#include "../inc/dp32g030/gpio.h"
#include "../inc/dp32g030/syscon.h"
#include "../scheduler.h"
#include "../settings.h"
#include "aes.h"
#include "bk4819-regs.h"
#include "bk4819.h"
#include "crc.h"
#include "eeprom.h"
#include "gpio.h"
#include "uart.h"
#include <stdbool.h>
#include <string.h>

static const char Version[] = "OSFW-fffffff";
static const char UART_Version[45] =
    "UV-K5 Firmware, Open Edition, OSFW-fffffff\r\n";

static bool UART_IsLogEnabled;
uint8_t UART_DMA_Buffer[256];

static bool bHasCustomAesKey = false;
static bool bIsInLockScreen = false;
static bool gIsLocked = false;
static uint8_t gTryCount;
static uint32_t gCustomAesKey[4] = {
    0xFFFFFFFFU,
    0xFFFFFFFFU,
    0xFFFFFFFFU,
    0xFFFFFFFFU,
};
static const uint32_t gDefaultAesKey[4] = {
    0x4AA5CC60,
    0x0312CC5F,
    0xFFD2DABB,
    0x6BBA7F92,
};
static uint32_t gChallenge[4];

void UART_Init(void) {
  uint32_t Delta;
  uint32_t Positive;
  uint32_t Frequency;

  UART1->CTRL =
      (UART1->CTRL & ~UART_CTRL_UARTEN_MASK) | UART_CTRL_UARTEN_BITS_DISABLE;
  Delta = SYSCON_RC_FREQ_DELTA;
  Positive = (Delta & SYSCON_RC_FREQ_DELTA_RCHF_SIG_MASK) >>
             SYSCON_RC_FREQ_DELTA_RCHF_SIG_SHIFT;
  Frequency = (Delta & SYSCON_RC_FREQ_DELTA_RCHF_DELTA_MASK) >>
              SYSCON_RC_FREQ_DELTA_RCHF_DELTA_SHIFT;
  if (Positive) {
    Frequency += 48000000U;
  } else {
    Frequency = 48000000U - Frequency;
  }

  UART1->BAUD = Frequency / 39053U;
  UART1->CTRL = UART_CTRL_RXEN_BITS_ENABLE | UART_CTRL_TXEN_BITS_ENABLE |
                UART_CTRL_RXDMAEN_BITS_ENABLE;
  UART1->RXTO = 4;
  UART1->FC = 0;
  UART1->FIFO = UART_FIFO_RF_LEVEL_BITS_8_BYTE | UART_FIFO_RF_CLR_BITS_ENABLE |
                UART_FIFO_TF_CLR_BITS_ENABLE;
  UART1->IE = 0;

  DMA_CTR = (DMA_CTR & ~DMA_CTR_DMAEN_MASK) | DMA_CTR_DMAEN_BITS_DISABLE;

  DMA_CH0->MSADDR = (uint32_t)(uintptr_t)&UART1->RDR;
  DMA_CH0->MDADDR = (uint32_t)(uintptr_t)UART_DMA_Buffer;
  DMA_CH0->MOD = 0
                 // Source
                 | DMA_CH_MOD_MS_ADDMOD_BITS_NONE |
                 DMA_CH_MOD_MS_SIZE_BITS_8BIT |
                 DMA_CH_MOD_MS_SEL_BITS_HSREQ_MS1
                 // Destination
                 | DMA_CH_MOD_MD_ADDMOD_BITS_INCREMENT |
                 DMA_CH_MOD_MD_SIZE_BITS_8BIT | DMA_CH_MOD_MD_SEL_BITS_SRAM;
  DMA_INTEN = 0;
  DMA_INTST =
      0 | DMA_INTST_CH0_TC_INTST_BITS_SET | DMA_INTST_CH1_TC_INTST_BITS_SET |
      DMA_INTST_CH2_TC_INTST_BITS_SET | DMA_INTST_CH3_TC_INTST_BITS_SET |
      DMA_INTST_CH0_THC_INTST_BITS_SET | DMA_INTST_CH1_THC_INTST_BITS_SET |
      DMA_INTST_CH2_THC_INTST_BITS_SET | DMA_INTST_CH3_THC_INTST_BITS_SET;
  DMA_CH0->CTR = 0 | DMA_CH_CTR_CH_EN_BITS_ENABLE |
                 ((0xFF << DMA_CH_CTR_LENGTH_SHIFT) & DMA_CH_CTR_LENGTH_MASK) |
                 DMA_CH_CTR_LOOP_BITS_ENABLE | DMA_CH_CTR_PRI_BITS_MEDIUM;
  UART1->IF = UART_IF_RXTO_BITS_SET;

  DMA_CTR = (DMA_CTR & ~DMA_CTR_DMAEN_MASK) | DMA_CTR_DMAEN_BITS_ENABLE;

  UART1->CTRL |= UART_CTRL_UARTEN_BITS_ENABLE;
}

void UART_Send(const void *pBuffer, uint32_t Size) {
  const uint8_t *pData = (const uint8_t *)pBuffer;
  uint32_t i;

  for (i = 0; i < Size; i++) {
    UART1->TDR = pData[i];
    while ((UART1->IF & UART_IF_TXFIFO_FULL_MASK) !=
           UART_IF_TXFIFO_FULL_BITS_NOT_SET) {
    }
  }
}

void UART_LogSend(const void *pBuffer, uint32_t Size) {
  if (UART_IsLogEnabled) {
    UART_Send(pBuffer, Size);
  }
}

#define DMA_INDEX(x, y) (((x) + (y)) % sizeof(UART_DMA_Buffer))

typedef struct {
  uint16_t ID;
  uint16_t Size;
} Header_t;

typedef struct {
  uint8_t Padding[2];
  uint16_t ID;
} Footer_t;

typedef struct {
  Header_t Header;
  uint32_t Timestamp;
} CMD_0514_t;

typedef struct {
  Header_t Header;
  struct {
    char Version[16];
    bool bHasCustomAesKey;
    bool bIsInLockScreen;
    uint8_t Padding[2];
    uint32_t Challenge[4];
  } Data;
} REPLY_0514_t;

typedef struct {
  Header_t Header;
  uint16_t Offset;
  uint8_t Size;
  uint8_t Padding;
  uint32_t Timestamp;
} CMD_051B_t;

typedef struct {
  Header_t Header;
  struct {
    uint16_t Offset;
    uint8_t Size;
    uint8_t Padding;
    uint8_t Data[128];
  } Data;
} REPLY_051B_t;

typedef struct {
  Header_t Header;
  uint16_t Offset;
  uint8_t Size;
  bool bAllowPassword;
  uint32_t Timestamp;
  uint8_t Data[0];
} CMD_051D_t;

typedef struct {
  Header_t Header;
  struct {
    uint16_t Offset;
  } Data;
} REPLY_051D_t;

typedef struct {
  Header_t Header;
  struct {
    uint16_t RSSI;
    uint8_t ExNoiseIndicator;
    uint8_t GlitchIndicator;
  } Data;
} REPLY_0527_t;

typedef struct {
  Header_t Header;
  struct {
    uint16_t Voltage;
    uint16_t Current;
  } Data;
} REPLY_0529_t;

typedef struct {
  Header_t Header;
  uint32_t Response[4];
} CMD_052D_t;

typedef struct {
  Header_t Header;
  struct {
    bool bIsLocked;
    uint8_t Padding[3];
  } Data;
} REPLY_052D_t;

typedef struct {
  Header_t Header;
  uint32_t Timestamp;
} CMD_052F_t;

typedef struct {
  Header_t Header;
  uint8_t RegNum;
} CMD_0601_t;

typedef struct {
  Header_t Header;
  struct {
    uint16_t Val;
    uint8_t v1;
    uint8_t v2;
  } Data;
} REPLY_0601_t;

typedef struct {
  Header_t Header;
  uint8_t RegNum;
  uint16_t RegValue;
} CMD_0602_t;

typedef struct {
  Header_t Header;
  struct {
    uint16_t Val;
    uint8_t v1;
    uint8_t v2;
  } Data;
} REPLY_0602_t;

typedef struct {
  Header_t Header;
  uint16_t Offset;
  uint32_t Timestamp;
} CMD_0701_t;

typedef struct {
  Header_t Header;
  struct {
    uint16_t Offset;
    uint8_t Data[sizeof(CH)];
  } Data;
} REPLY_0702_t;

static const uint8_t Obfuscation[16] = {0x16, 0x6C, 0x14, 0xE6, 0x2E, 0x91,
                                        0x0D, 0x40, 0x21, 0x35, 0xD5, 0x40,
                                        0x13, 0x03, 0xE9, 0x80};

static union {
  uint8_t Buffer[256];
  struct {
    Header_t Header;
    uint8_t Data[252];
  };
} UART_Command;

static uint32_t Timestamp;
static uint16_t gUART_WriteIndex;
static bool bIsEncrypted = true;

static Header_t Header;
static Footer_t Footer;
static uint8_t *pBytes;
static void SendReply(void *pReply, uint16_t Size) {
  uint16_t i;

  if (bIsEncrypted) {
    pBytes = (uint8_t *)pReply;
    for (i = 0; i < Size; i++) {
      pBytes[i] ^= Obfuscation[i % 16];
    }
  }

  Header.ID = 0xCDAB;
  Header.Size = Size;
  UART_Send(&Header, sizeof(Header));
  UART_Send(pReply, Size);
  if (bIsEncrypted) {
    Footer.Padding[0] = Obfuscation[(Size + 0) % 16] ^ 0xFF;
    Footer.Padding[1] = Obfuscation[(Size + 1) % 16] ^ 0xFF;
  } else {
    Footer.Padding[0] = 0xFF;
    Footer.Padding[1] = 0xFF;
  }
  Footer.ID = 0xBADC;

  UART_Send(&Footer, sizeof(Footer));
}

static void SendVersion(void) {
  REPLY_0514_t Reply;

  Reply.Header.ID = 0x0515;
  Reply.Header.Size = sizeof(Reply.Data);
  strcpy(Reply.Data.Version, Version);
  Reply.Data.bHasCustomAesKey = bHasCustomAesKey;
  Reply.Data.bIsInLockScreen = bIsInLockScreen;
  Reply.Data.Challenge[0] = gChallenge[0];
  Reply.Data.Challenge[1] = gChallenge[1];
  Reply.Data.Challenge[2] = gChallenge[2];
  Reply.Data.Challenge[3] = gChallenge[3];

  SendReply(&Reply, sizeof(Reply));
}

static bool IsBadChallenge(const uint32_t *pKey, const uint32_t *pIn,
                           const uint32_t *pResponse) {
  uint8_t i;
  uint32_t IV[4];

  IV[0] = 0;
  IV[1] = 0;
  IV[2] = 0;
  IV[3] = 0;
  AES_Encrypt(pKey, IV, pIn, IV, true);
  for (i = 0; i < 4; i++) {
    if (IV[i] != pResponse[i]) {
      return true;
    }
  }

  return false;
}

static void CMD_0514(const uint8_t *pBuffer) {
  const CMD_0514_t *pCmd = (const CMD_0514_t *)pBuffer;

  Timestamp = pCmd->Timestamp;
  GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_BACKLIGHT);
  SendVersion();
}

static void CMD_051B(const uint8_t *pBuffer) {
  const CMD_051B_t *pCmd = (const CMD_051B_t *)pBuffer;
  REPLY_051B_t Reply;
  bool bLocked = false;

  if (pCmd->Timestamp != Timestamp) {
    return;
  }

  memset(&Reply, 0, sizeof(Reply));
  Reply.Header.ID = 0x051C;
  Reply.Header.Size = pCmd->Size + 4;
  Reply.Data.Offset = pCmd->Offset;
  Reply.Data.Size = pCmd->Size;

  if (bHasCustomAesKey) {
    bLocked = gIsLocked;
  }

  if (!bLocked) {
    EEPROM_ReadBuffer(pCmd->Offset, Reply.Data.Data, pCmd->Size);
  }

  SendReply(&Reply, pCmd->Size + 8);
}

static void CMD_051D(const uint8_t *pBuffer) {
  const CMD_051D_t *pCmd = (const CMD_051D_t *)pBuffer;
  REPLY_051D_t Reply;
  bool bReloadEeprom;
  bool bIsLocked;

  if (pCmd->Timestamp != Timestamp) {
    return;
  }

  bReloadEeprom = false;

  Reply.Header.ID = 0x051E;
  Reply.Header.Size = sizeof(Reply.Data);
  Reply.Data.Offset = pCmd->Offset;

  bIsLocked = bHasCustomAesKey;
  if (bHasCustomAesKey) {
    bIsLocked = gIsLocked;
  }

  if (!bIsLocked) {
    uint16_t i;

    for (i = 0; i < (pCmd->Size / 8U); i++) {
      uint16_t Offset = pCmd->Offset + (i * 8U);

      if (Offset >= 0x0F30 && Offset < 0x0F40) {
        if (!gIsLocked) {
          bReloadEeprom = true;
        }
      }

      if ((Offset < 0x0E98 || Offset >= 0x0EA0) || !bIsInLockScreen ||
          pCmd->bAllowPassword) {
        EEPROM_WriteBuffer(Offset, &pCmd->Data[i * 8U], 8);
      }
    }

    /* if (bReloadEeprom) {
      BOARD_EEPROM_Init();
    } */
  }

  SendReply(&Reply, sizeof(Reply));
}

static void CMD_061D(const uint8_t *pBuffer) {
  const CMD_051D_t *pCmd = (const CMD_051D_t *)pBuffer;
  REPLY_051D_t Reply;
  bool bReloadEeprom;
  bool bIsLocked;

  if (pCmd->Timestamp != Timestamp) {
    return;
  }

  bReloadEeprom = false;

  Reply.Header.ID = 0x061E;
  Reply.Header.Size = sizeof(Reply.Data);
  Reply.Data.Offset = pCmd->Offset;

  bIsLocked = bHasCustomAesKey;
  if (bHasCustomAesKey) {
    bIsLocked = gIsLocked;
  }

  const uint32_t EEPROM_SIZE = SETTINGS_GetEEPROMSize();
  const uint32_t PATCH_START = EEPROM_SIZE - PATCH_SIZE;

  for (uint16_t i = 0; i < (pCmd->Size / 8U); i++) {
    uint32_t Offset = PATCH_START + pCmd->Offset + (i * 8U);

    if (Offset >= 0x0F30 && Offset < 0x0F40) {
      if (!gIsLocked) {
        bReloadEeprom = true;
      }
    }

    if ((Offset < 0x0E98 || Offset >= 0x0EA0) || !bIsInLockScreen ||
        pCmd->bAllowPassword) {
      EEPROM_WriteBuffer(Offset, &pCmd->Data[i * 8U], 8);
    }
  }

  SendReply(&Reply, sizeof(Reply));
}

static void CMD_0527(void) {
  REPLY_0527_t Reply;

  Reply.Header.ID = 0x0528;
  Reply.Header.Size = sizeof(Reply.Data);
  Reply.Data.RSSI = BK4819_ReadRegister(BK4819_REG_67) & 0x01FF;
  Reply.Data.ExNoiseIndicator = BK4819_ReadRegister(BK4819_REG_65) & 0x007F;
  Reply.Data.GlitchIndicator = BK4819_ReadRegister(BK4819_REG_63);

  SendReply(&Reply, sizeof(Reply));
}

static void CMD_0529(void) {
  REPLY_0529_t Reply;

  Reply.Header.ID = 0x52A;
  Reply.Header.Size = sizeof(Reply.Data);
  // Original doesn't actually send current!
  BOARD_ADC_GetBatteryInfo(&Reply.Data.Voltage, &Reply.Data.Current);
  SendReply(&Reply, sizeof(Reply));
}

static void CMD_052D(const uint8_t *pBuffer) {
  const CMD_052D_t *pCmd = (const CMD_052D_t *)pBuffer;
  REPLY_052D_t Reply;
  bool bIsLocked;

  Reply.Header.ID = 0x052E;
  Reply.Header.Size = sizeof(Reply.Data);

  bIsLocked = bHasCustomAesKey;

  if (!bIsLocked) {
    bIsLocked = IsBadChallenge(gCustomAesKey, gChallenge, pCmd->Response);
  }
  if (!bIsLocked) {
    bIsLocked = IsBadChallenge(gDefaultAesKey, gChallenge, pCmd->Response);
    if (bIsLocked) {
      gTryCount++;
    }
  }
  if (gTryCount < 3) {
    if (!bIsLocked) {
      gTryCount = 0;
    }
  } else {
    gTryCount = 3;
    bIsLocked = true;
  }
  gIsLocked = bIsLocked;
  Reply.Data.bIsLocked = bIsLocked;
  SendReply(&Reply, sizeof(Reply));
}

static void CMD_052F(const uint8_t *pBuffer) {
  const CMD_052F_t *pCmd = (const CMD_052F_t *)pBuffer;

  /* gEeprom.DUAL_WATCH = DUAL_WATCH_OFF;
  gEeprom.CROSS_BAND_RX_TX = CROSS_BAND_OFF;
  gEeprom.RX_VFO = 0;
  gEeprom.DTMF_SIDE_TONE = false;
  gEeprom.VfoInfo[0].FrequencyReverse = false;
  gEeprom.VfoInfo[0].pRX = &gEeprom.VfoInfo[0].ConfigRX;
  gEeprom.VfoInfo[0].pTX = &gEeprom.VfoInfo[0].ConfigTX;
  gEeprom.VfoInfo[0].OFFSET_DIR = FREQUENCY_DEVIATION_OFF;
  gEeprom.VfoInfo[0].DTMF_PTT_ID_TX_MODE = PTT_ID_OFF;
  gEeprom.VfoInfo[0].DTMF_DECODING_ENABLE = false;
  if (gCurrentFunction == FUNCTION_POWER_SAVE) {
    FUNCTION_Select(FUNCTION_FOREGROUND);
  } */
  Timestamp = pCmd->Timestamp;
  GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_BACKLIGHT);

  SendVersion();
}

#ifdef ENABLE_UART_CAT

static void CMD_0601(const uint8_t *pBuffer) {
  const CMD_0601_t *pCmd = (const CMD_0601_t *)pBuffer;
  REPLY_0601_t Reply;

  Reply.Header.ID = 0x0601;
  Reply.Header.Size = sizeof(Reply.Data);
  Reply.Data.Val = BK4819_ReadRegister(pCmd->RegNum);
  Reply.Data.v1 = pCmd->RegNum;

  SendReply(&Reply, sizeof(Reply));
}

static void CMD_0602(const uint8_t *pBuffer) {
  const CMD_0602_t *pCmd = (const CMD_0602_t *)pBuffer;
  REPLY_0602_t Reply;

  Reply.Header.ID = 0x0602;
  Reply.Header.Size = sizeof(Reply.Data);
  BK4819_WriteRegister(pCmd->RegNum, pCmd->RegValue);
  Reply.Data.Val = BK4819_ReadRegister(pCmd->RegNum);
  Reply.Data.v1 = pCmd->RegNum;

  SendReply(&Reply, sizeof(Reply));
}

#endif

#include "../helper/channels.h"
static void CMD_0701(const uint8_t *pBuffer) {
  const CMD_0701_t *pCmd = (const CMD_0701_t *)pBuffer;
  REPLY_0702_t Reply;

  memset(&Reply, 0, sizeof(Reply));
  Reply.Header.ID = 0x0702;
  Reply.Data.Offset = pCmd->Offset;
  Reply.Header.Size = sizeof(Reply.Data);
  CH ch;
  CHANNELS_Load(pCmd->Offset, &ch);
  memcpy(Reply.Data.Data, &ch, sizeof(ch));

  SendReply(&Reply, sizeof(Reply));
}

uint64_t xtou64(const char *str) {
  uint64_t res = 0;
  char c;

  while ((c = *str++)) {
    char v = ((c & 0xF) + (c >> 6)) | ((c >> 3) & 0x8);
    res = (res << 4) | (uint64_t)v;
  }

  return res;
}

bool UART_IsCommandAvailable(void) {
  uint16_t DmaLength;
  uint16_t CommandLength;
  uint16_t Index;
  uint16_t TailIndex;
  uint16_t Size;
  uint16_t CRC;
  uint16_t i;

  DmaLength = DMA_CH0->ST & 0xFFFU;
  while (1) {
    if (gUART_WriteIndex == DmaLength) {
      return false;
    }

    while (gUART_WriteIndex != DmaLength &&
           UART_DMA_Buffer[gUART_WriteIndex] != 0xABU) {
      gUART_WriteIndex = DMA_INDEX(gUART_WriteIndex, 1);
    }

    if (gUART_WriteIndex == DmaLength) {
      return false;
    }

    if (gUART_WriteIndex < DmaLength) {
      CommandLength = DmaLength - gUART_WriteIndex;
    } else {
      CommandLength = (DmaLength + sizeof(UART_DMA_Buffer)) - gUART_WriteIndex;
    }
    if (CommandLength < 8) {
      return 0;
    }
    if (UART_DMA_Buffer[DMA_INDEX(gUART_WriteIndex, 1)] == 0xCD) {
      break;
    }
    gUART_WriteIndex = DMA_INDEX(gUART_WriteIndex, 1);
  }

  Index = DMA_INDEX(gUART_WriteIndex, 2);
  Size = (UART_DMA_Buffer[DMA_INDEX(Index, 1)] << 8) | UART_DMA_Buffer[Index];
  if (Size + 8 > sizeof(UART_DMA_Buffer)) {
    gUART_WriteIndex = DmaLength;
    return false;
  }
  if (CommandLength < Size + 8) {
    return false;
  }
  Index = DMA_INDEX(Index, 2);
  TailIndex = DMA_INDEX(Index, Size + 2);
  if (UART_DMA_Buffer[TailIndex] != 0xDC ||
      UART_DMA_Buffer[DMA_INDEX(TailIndex, 1)] != 0xBA) {
    gUART_WriteIndex = DmaLength;
    return false;
  }
  if (TailIndex < Index) {
    uint16_t ChunkSize = sizeof(UART_DMA_Buffer) - Index;

    memcpy(UART_Command.Buffer, UART_DMA_Buffer + Index, ChunkSize);
    memcpy(UART_Command.Buffer + ChunkSize, UART_DMA_Buffer, TailIndex);
  } else {
    memcpy(UART_Command.Buffer, UART_DMA_Buffer + Index, TailIndex - Index);
  }

  TailIndex = DMA_INDEX(TailIndex, 2);
  if (TailIndex < gUART_WriteIndex) {
    memset(UART_DMA_Buffer + gUART_WriteIndex, 0,
           sizeof(UART_DMA_Buffer) - gUART_WriteIndex);
    memset(UART_DMA_Buffer, 0, TailIndex);
  } else {
    memset(UART_DMA_Buffer + gUART_WriteIndex, 0, TailIndex - gUART_WriteIndex);
  }

  gUART_WriteIndex = TailIndex;

  if (UART_Command.Header.ID == 0x0514) {
    bIsEncrypted = false;
  }
  if (UART_Command.Header.ID == 0x6902) {
    bIsEncrypted = true;
  }

  if (bIsEncrypted) {
    for (i = 0; i < Size + 2; i++) {
      UART_Command.Buffer[i] ^= Obfuscation[i % 16];
    }
  }

  CRC = UART_Command.Buffer[Size] | (UART_Command.Buffer[Size + 1] << 8);
  if (CRC_Calculate(UART_Command.Buffer, Size) != CRC) {
    return false;
  }

  return true;
}

void UART_HandleCommand(void) {
  switch (UART_Command.Header.ID) {
  case 0x0514:
    CMD_0514(UART_Command.Buffer);
    break;

  case 0x051B:
    CMD_051B(UART_Command.Buffer);
    break;

  case 0x051D:
    CMD_051D(UART_Command.Buffer);
    break;

  case 0x051F:
    // Not implementing non-authentic command
    break;

  case 0x0521:
    // Not implementing non-authentic command
    break;

  case 0x0527:
    CMD_0527();
    break;

  case 0x0529:
    CMD_0529();
    break;

  case 0x052D:
    CMD_052D(UART_Command.Buffer);
    break;

  case 0x052F:
    CMD_052F(UART_Command.Buffer);
    break;

  case 0x05DD:
    NVIC_SystemReset();
    break;

    // write patch
  case 0x061D:
    CMD_061D(UART_Command.Buffer);
    break;

    // read ch
  case 0x0701:
    CMD_0701(UART_Command.Buffer);
    break;
  }
}

void UART_printf(const char *str, ...) {
  char text[128];
  int len;

  va_list va;
  va_start(va, str);
  len = vsnprintf(text, sizeof(text), str, va);
  va_end(va);

  UART_Send(text, len);
}

void Log(const char *pattern, ...) {
  char text[128];
  va_list args;
  va_start(args, pattern);
  vsnprintf(text, sizeof(text), pattern, args);
  va_end(args);
  UART_printf("%u %s\n", Now(), text);
}
