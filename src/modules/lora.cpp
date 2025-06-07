#include <vector>
#include <cstdint>
#include <cstring>
#include <map>
#include <chrono>
#include <thread>
#include <string>
#include "sx126x.h"
#include "lora.h"

// GLOBAL VARIABLES

static constexpr auto AIRTIME         = std::chrono::milliseconds(400);
static constexpr auto MARGIN          = std::chrono::milliseconds(50);
static constexpr auto PACKET_INTERVAL = AIRTIME + MARGIN; // ~450 ms
static constexpr auto MSG_TIMEOUT     = 5000; // 5s, considering max 10packets per message

static std::vector<std::vector<uint8_t>> preparePackets(const uint8_t* data, size_t length) {
    uint16_t total = static_cast<uint16_t>((length + MAX_PAYLOAD - 1) / MAX_PAYLOAD);
    static uint16_t nextMessageId = 0;
    uint16_t msgId = nextMessageId++;

    std::vector<std::vector<uint8_t>> packets;
    packets.reserve(total);

    size_t offset = 0;
    for (uint16_t pid = 0; pid < total; ++pid) {
        size_t chunkSize = std::min(MAX_PAYLOAD, length - offset);
        PacketHeader hdr{};
        hdr.messageId = msgId;
        hdr.packetId = pid;
        hdr.totalPackets = (pid == 0 ? total : 0);
        hdr.type = (pid == 0) ? PT_START : (pid + 1 == total ? PT_END : PT_MIDDLE);

        std::vector<uint8_t> pkt(sizeof(hdr) + chunkSize);
        std::memcpy(pkt.data(), &hdr, sizeof(hdr));
        std::memcpy(pkt.data() + sizeof(hdr), data + offset, chunkSize);
        packets.push_back(std::move(pkt));

        offset += chunkSize;
    }
    return packets;
}

bool sendOverLora(std::string str) {
    sx126x lora("/dev/ttyS0", 868, 0x1234, 22, false, 2400, 0, 240, 0, false, false, false);

    std::vector<uint8_t> message(str.begin(), str.end());

    auto packets = preparePackets(message.data(), message.size());
    for (auto& pkt : packets) {
        lora.send(pkt);
        std::this_thread::sleep_for(PACKET_INTERVAL);
    }
    return true;
}

bool receiveOverLora() {
    sx126x lora("/dev/ttyS0", 868, 0x1234, 22, false, 2400, 0, 240, 0, false, false, false);

    using Clock = std::chrono::steady_clock;
    auto startTime = Clock::now();
    uint32_t timeoutMs = MSG_TIMEOUT;

    struct Assembly {
        uint16_t total = 0;
        std::map<uint16_t, std::vector<uint8_t>> parts;
    };
    std::map<uint16_t, Assembly> buffers;
    std::vector<uint8_t> outMessage;

    while (true) {
        std::string raw = lora.receive();
        if (raw.size() <= sizeof(PacketHeader)) {
        } else {
            PacketHeader hdr;
            std::memcpy(&hdr, raw.data(), sizeof(hdr));
            size_t payloadLen = raw.size() - sizeof(hdr);
            const uint8_t* payload = reinterpret_cast<const uint8_t*>(raw.data() + sizeof(hdr));

            auto& assembly = buffers[hdr.messageId];
            if (hdr.type == PT_START) {
                assembly.total = hdr.totalPackets;
            }
            assembly.parts[hdr.packetId] = std::vector<uint8_t>(payload, payload + payloadLen);

            if (assembly.total > 0 && assembly.parts.size() == assembly.total) {
                outMessage.clear();
                for (uint16_t i = 0; i < assembly.total; ++i) {
                    auto& chunk = assembly.parts[i];
                    outMessage.insert(outMessage.end(), chunk.begin(), chunk.end());
                }
                return true;
            }
        }
        if (std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - startTime).count() > timeoutMs) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
