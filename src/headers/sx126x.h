#ifndef SX126X_H
#define SX126X_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <array>

/**
 * A C++ driver for the Waveshare SX1268 LoRa HAT,
 * originally converted from a Python implementation.
 *
 * Before using any sx126x instance, be sure to call gpioInitialise()
 * (and call gpioTerminate() before your program exits).
 */
class sx126x {
public:
    // GPIO pin numbers (BCM numbering)
    static constexpr int M0 = 22;
    static constexpr int M1 = 27;

    // Constructor / Destructor
    sx126x(const std::string& serial_num,
           int freq,
           uint16_t addr,
           int power,
           bool rssi,
           int air_speed    = 2400,
           int net_id       = 0,
           int buffer_size  = 240,
           uint16_t crypt   = 0,
           bool relay       = false,
           bool lbt         = false,
           bool wor         = false);

    ~sx126x();

    // Re-configure the module (can be called again to change frequency, power, etc.)
    void set(int freq,
             uint16_t addr,
             int power,
             bool rssi,
             int air_speed    = 2400,
             int net_id       = 0,
             int buffer_size  = 240,
             uint16_t crypt   = 0,
             bool relay       = false,
             bool lbt         = false,
             bool wor         = false);

    // Query and print the current register settings
    void get_settings();

    // Send a raw packet (formatted as: [dest_high, dest_low, freq_offset, payload…])
    void send(const std::vector<uint8_t>& data);

    // Check UART for incoming LoRa data, parse & print it (+RSSI if enabled)
    void receive();

    // Request and print noise RSSI (and optional last-packet RSSI)
    void get_channel_rssi();

    // Public attributes (read-only or directly modifiable as needed)
    bool rssi;              // whether to append a packet-RSSI byte
    uint16_t addr;          // this node’s address
    int       start_freq;   // base frequency (410 or 850)
    int       offset_freq;  // offset from start_freq in MHz
    std::string serial_n;   // e.g. "/dev/ttyS0"

private:
    // Internal UART handle (pigpio’s serialOpen returns an int)
    int      serial_handle;
    uint32_t baudrate;

    int      power;        // TX power (22/17/13/10 dBm)
    int      freq;         // center frequency in MHz
    int      send_to;      // “destination address”

    // 12-byte configuration buffer
    std::array<uint8_t,12> cfg_reg;

    // Buffer to hold “get settings” response
    std::vector<uint8_t>   get_reg;

    // Lookup tables (dictionaries) for air-speed, power, buffer-size
    std::map<int,uint8_t>  lora_air_speed_dic;
    std::map<int,uint8_t>  lora_power_dic;
    std::map<int,uint8_t>  lora_buffer_size_dic;

    // UART & package-size masks:
    static constexpr uint8_t SX126X_UART_BAUDRATE_1200   = 0x00;
    static constexpr uint8_t SX126X_UART_BAUDRATE_2400   = 0x20;
    static constexpr uint8_t SX126X_UART_BAUDRATE_4800   = 0x40;
    static constexpr uint8_t SX126X_UART_BAUDRATE_9600   = 0x60;
    static constexpr uint8_t SX126X_UART_BAUDRATE_19200  = 0x80;
    static constexpr uint8_t SX126X_UART_BAUDRATE_38400  = 0xA0;
    static constexpr uint8_t SX126X_UART_BAUDRATE_57600  = 0xC0;
    static constexpr uint8_t SX126X_UART_BAUDRATE_115200 = 0xE0;

    static constexpr uint8_t SX126X_PACKAGE_SIZE_240_BYTE = 0x00;
    static constexpr uint8_t SX126X_PACKAGE_SIZE_128_BYTE = 0x40;
    static constexpr uint8_t SX126X_PACKAGE_SIZE_64_BYTE  = 0x80;
    static constexpr uint8_t SX126X_PACKAGE_SIZE_32_BYTE  = 0xC0;

    static constexpr uint8_t SX126X_Power_22dBm = 0x00;
    static constexpr uint8_t SX126X_Power_17dBm = 0x01;
    static constexpr uint8_t SX126X_Power_13dBm = 0x02;
    static constexpr uint8_t SX126X_Power_10dBm = 0x03;

    // (No other public methods or data members)
};

#endif // SX126X_H
