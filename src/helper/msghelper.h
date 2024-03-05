#ifndef MSG_HELPER_H
#define MSG_HELPER_H
#include <stdint.h>

#define NONCE_LENGTH 13
#define NICK_LENGTH 10
#define PAYLOAD_LENGTH 30

typedef enum ModemModulation {
  MOD_FSK_450,
  MOD_FSK_700,
  MOD_AFSK_1200,
} ModemModulation;

typedef enum MsgStatus {
  READY,
  SENDING,
  RECEIVING,
} MsgStatus;

typedef enum PacketType {
  MESSAGE_PACKET = 100u,
  ENCRYPTED_MESSAGE_PACKET,
  ACK_PACKET,
  INVALID_PACKET
} PacketType;

typedef union {
  struct {
    uint8_t header;
    uint8_t nick[NICK_LENGTH];
    uint8_t payload[PAYLOAD_LENGTH];
    unsigned char nonce[NONCE_LENGTH];
  } data;
  // header + payload + nonce = must be an even number
  uint8_t buf[1 + NICK_LENGTH + PAYLOAD_LENGTH + NONCE_LENGTH];
} DataPacket;

void MSG_StorePacket(const uint16_t intBits);
void MSG_Send(const char *message);
DataPacket *MSG_GetMessage(uint8_t i);
void MSG_Init();

#endif /* end of include guard: MSG_H */
