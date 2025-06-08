#ifndef LORA_H
#define LORA_H

#include <vector>
#include <cstdint>
#include "sx126x.h"

// Maximum payload size per packet
static const size_t MAX_PAYLOAD = 200;

// Packet types for framing
enum PacketType : uint8_t {
    PT_START = 0x01,
    PT_MIDDLE = 0x02,
    PT_END = 0x03
};

#pragma pack(push, 1)
struct PacketHeader {
    PacketType type;
    uint16_t messageId;
    uint16_t packetId;
    uint16_t totalPackets;
};
#pragma pack(pop)



// Send a large message over LoRa
bool sendOverLora(std::string message);

// Receive a full message over LoRa
bool receiveOverLora();

#endif // LORA_H
