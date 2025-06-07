// sx126x_driver.cpp
//
// C++ rewrite of the Python SX1268 driver code.
// This implementation uses the pigpio library for GPIO and serial operations.
// Make sure to link against libpigpio (e.g., compile with `-lpigpio -lrt -lpthread`).
//
// Before using any sx126x instance, call `gpioInitialise()` in your main()
// (and call `gpioTerminate()` before exiting your program).
//
// Note: Error checking (e.g., return codes from pigpio functions) is minimal
//       for clarity. In production code, you should check for failures and handle them.

#include "sx126x.h"

#include <iostream>
#include <thread>
#include <chrono>

// #include <pigpio.h>    // pigpio for GPIO + UART
extern "C" {
    #include <pigpio.h>
}

void serialFlush(int handle) {
    while (serDataAvailable(handle) > 0) {
        serReadByte(handle); // discard incoming data
    }
}
// -------------------- Constructor & Destructor --------------------

sx126x::sx126x(const std::string& serial_num,
               int freq,
               uint16_t addr,
               int power,
               bool rssi,
               int air_speed,
               int net_id,
               int buffer_size,
               uint16_t crypt,
               bool relay,
               bool lbt,
               bool wor)
    : rssi(rssi),
      addr(addr),
      serial_n(serial_num),
      power(power),
      freq(freq),
      baudrate(9600),
      cfg_reg{ 0xC2,0x00,0x09,0x00,0x00,0x00,0x62,0x00,0x12,0x43,0x00,0x00 },
      // initialize dictionaries:
      lora_air_speed_dic{
        {1200, 0x01},
        {2400, 0x02},
        {4800, 0x03},
        {9600, 0x04},
        {19200,0x05},
        {38400,0x06},
        {62500,0x07}
      },
      lora_power_dic{
        {22, 0x00},
        {17, 0x01},
        {13, 0x02},
        {10, 0x03}
      },
      lora_buffer_size_dic{
        {240, SX126X_PACKAGE_SIZE_240_BYTE},
        {128, SX126X_PACKAGE_SIZE_128_BYTE},
        {64,  SX126X_PACKAGE_SIZE_64_BYTE},
        {32,  SX126X_PACKAGE_SIZE_32_BYTE}
      }
{
    // Initialize GPIO (M0 / M1) to output mode
    gpioSetMode(M0, PI_OUTPUT);
    gpioSetMode(M1, PI_OUTPUT);

    // Ensure M0=LOW, M1=HIGH to enter “configuration” mode initially
    gpioWrite(M0, PI_LOW);
    gpioWrite(M1, PI_HIGH);

    // Open UART at 9600 baud (for initial config)
    serial_handle = serOpen(const_cast<char*>(serial_n.c_str()), baudrate, 0);
    if (serial_handle < 0) {
        std::cerr << "Failed to open serial port " << serial_n << std::endl;
        return;
    }
    serialFlush(serial_handle);

    // Apply initial settings
    set(freq, addr, power, rssi, air_speed, net_id, buffer_size, crypt, relay, lbt, wor);
}

sx126x::~sx126x() {
    if (serial_handle >= 0) {
        serClose(serial_handle);
    }
}

// -------------------- set(...) Method --------------------

void sx126x::set(int freq,
                 uint16_t addr,
                 int power,
                 bool rssi,
                 int air_speed,
                 int net_id,
                 int buffer_size,
                 uint16_t crypt,
                 bool relay,
                 bool lbt,
                 bool wor)
{
    send_to = addr;
    this->addr = addr;
    this->power = power;
    this->freq = freq;
    this->rssi = rssi;

    // Enter “configuration” mode: M0=0, M1=1
    gpioWrite(M0, PI_LOW);
    gpioWrite(M1, PI_HIGH);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Split address into high / low byte
    uint8_t low_addr  = static_cast<uint8_t>(addr & 0x00FF);
    uint8_t high_addr = static_cast<uint8_t>((addr >> 8) & 0x00FF);
    uint8_t net_id_temp = static_cast<uint8_t>(net_id & 0x00FF);

    // Determine frequency offset relative to base freq (410 or 850)
    uint8_t freq_temp = 0;
    if (freq > 850) {
        freq_temp = static_cast<uint8_t>(freq - 850);
        start_freq = 850;
        offset_freq = freq_temp;
    }
    else if (freq > 410) {
        freq_temp = static_cast<uint8_t>(freq - 410);
        start_freq = 410;
        offset_freq = freq_temp;
    }

    // Look up the air-speed code
    uint8_t air_speed_temp = 0x00;
    auto it_as = lora_air_speed_dic.find(air_speed);
    if (it_as != lora_air_speed_dic.end()) {
        air_speed_temp = it_as->second;
    }

    // Look up buffer-size code
    uint8_t buffer_size_temp = 0x00;
    auto it_bs = lora_buffer_size_dic.find(buffer_size);
    if (it_bs != lora_buffer_size_dic.end()) {
        buffer_size_temp = it_bs->second;
    }

    // Look up power code
    uint8_t power_temp = 0x00;
    auto it_pw = lora_power_dic.find(power);
    if (it_pw != lora_power_dic.end()) {
        power_temp = it_pw->second;
    }

    // RSSI bit (0x80 if we want packet-RSSI appended; 0x00 otherwise)
    uint8_t rssi_temp = (rssi ? 0x80 : 0x00);

    // Split 16-bit crypt key into high / low byte
    uint8_t h_crypt = static_cast<uint8_t>((crypt >> 8) & 0x00FF);
    uint8_t l_crypt = static_cast<uint8_t>(crypt & 0x00FF);

    if (!relay) {
        cfg_reg[3]  = high_addr;
        cfg_reg[4]  = low_addr;
        cfg_reg[5]  = net_id_temp;
        cfg_reg[6]  = SX126X_UART_BAUDRATE_9600 + air_speed_temp;
        // Byte 7: buffer_size + power + 0x20 (enable noise RSSI read)
        cfg_reg[7]  = static_cast<uint8_t>(buffer_size_temp + power_temp + 0x20);
        cfg_reg[8]  = freq_temp;
        // Byte 9: base=0x43, plus rssi_temp if packet-RSSI is desired
        cfg_reg[9]  = static_cast<uint8_t>(0x43 + rssi_temp);
        cfg_reg[10] = h_crypt;
        cfg_reg[11] = l_crypt;
    }
    else {
        // Relay mode uses fixed addresses 0x01,0x02,0x03 (example)
        cfg_reg[3]  = 0x01;
        cfg_reg[4]  = 0x02;
        cfg_reg[5]  = 0x03;
        cfg_reg[6]  = SX126X_UART_BAUDRATE_9600 + air_speed_temp;
        cfg_reg[7]  = static_cast<uint8_t>(buffer_size_temp + power_temp + 0x20);
        cfg_reg[8]  = freq_temp;
        cfg_reg[9]  = static_cast<uint8_t>(0x03 + rssi_temp);
        cfg_reg[10] = h_crypt;
        cfg_reg[11] = l_crypt;
    }

    serialFlush(serial_handle);

    // Try sending configuration up to 2 times if needed
    for (int attempt = 0; attempt < 2; ++attempt) {
        serWrite(serial_handle,
                    reinterpret_cast<char*>(cfg_reg.data()),
                    cfg_reg.size());
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        int available = serDataAvailable(serial_handle);
        if (available > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::vector<uint8_t> r_buff(available, 0);
            int read_bytes = serRead(serial_handle,
                                       reinterpret_cast<char*>(r_buff.data()),
                                       available);
            if (read_bytes > 0 && r_buff[0] == 0xC1) {
                std::cout << "Parameters setting is: ";
                for (auto byte : cfg_reg) {
                    std::cout << std::hex << static_cast<int>(byte) << " ";
                }
                std::cout << std::dec << std::endl;

                std::cout << "Parameters return is: ";
                for (auto byte : r_buff) {
                    std::cout << std::hex << static_cast<int>(byte) << " ";
                }
                std::cout << std::dec << std::endl;
            }
            else {
                std::cout << "Parameters setting fail: ";
                for (auto byte : r_buff) {
                    std::cout << std::hex << static_cast<int>(byte) << " ";
                }
                std::cout << std::dec << std::endl;
            }
            break;
        }
        else {
            std::cout << "Setting fail, setting again..." << std::endl;
            serialFlush(serial_handle);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            if (attempt == 1) {
                std::cout << "Setting fail, press Esc to exit and run again." << std::endl;
            }
        }
    }

    // Return to normal UART mode: M0=0, M1=0
    gpioWrite(M0, PI_LOW);
    gpioWrite(M1, PI_LOW);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// -------------------- get_settings() --------------------

void sx126x::get_settings() {
    // Enter “get setting” mode: M1=HIGH
    gpioWrite(M1, PI_HIGH);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Send the “get setting” command (3 bytes)
    uint8_t cmd[3] = { 0xC1, 0x00, 0x09 };
    serWrite(serial_handle, reinterpret_cast<char*>(cmd), 3);

    // Wait and read response
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    int available = serDataAvailable(serial_handle);
    if (available > 0) {
        get_reg.resize(available);
        serRead(serial_handle,
                   reinterpret_cast<char*>(get_reg.data()),
                   available);
    }

    // Parse & print if the response is valid
    if (get_reg.size() >= 9 && get_reg[0] == 0xC1 && get_reg[2] == 0x09) {
        uint8_t fre_temp    = get_reg[8];
        uint16_t addr_temp  = static_cast<uint16_t>(get_reg[3]) << 8
                            | static_cast<uint16_t>(get_reg[4]);
        uint8_t air_speed_temp = get_reg[6] & 0x03;
        uint8_t power_temp     = get_reg[7] & 0x03;

        std::cout << "Frequency is " << static_cast<int>(fre_temp)
                  << ".125 MHz" << std::endl;
        std::cout << "Node address is " << addr_temp << std::endl;

        // Reverse lookup air speed
        int air_rate = 0;
        for (auto& kv : lora_air_speed_dic) {
            if (kv.second == air_speed_temp) {
                air_rate = kv.first;
                break;
            }
        }
        std::cout << "Air speed is " << air_rate << " bps" << std::endl;

        // Reverse lookup power
        int power_dbm = 0;
        for (auto& kv : lora_power_dic) {
            if (kv.second == power_temp) {
                power_dbm = kv.first;
                break;
            }
        }
        std::cout << "Power is " << power_dbm << " dBm" << std::endl;

        // Exit “get setting” mode
        gpioWrite(M1, PI_LOW);
    }
    else {
        std::cout << "Failed to get settings or invalid response." << std::endl;
        gpioWrite(M1, PI_LOW);
    }
}

// -------------------- send(...) --------------------

void sx126x::send(const std::vector<uint8_t>& data) {
    // Ensure normal TX mode: M0=0, M1=0
    gpioWrite(M0, PI_LOW);
    gpioWrite(M1, PI_LOW);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    if (!data.empty()) {
        serWrite(serial_handle,
                    const_cast<char*>(reinterpret_cast<const char*>(data.data())),
                    data.size());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// -------------------- receive() --------------------

std::string sx126x::receive() {
    int available = serDataAvailable(serial_handle);
    if (available > 0) {
        // Give the module time to finish sending bytes
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        int new_available = serDataAvailable(serial_handle);
        if (new_available <= 0) return "";

        std::vector<uint8_t> r_buff(new_available, 0);
        int read_bytes = serRead(serial_handle,
                                   reinterpret_cast<char*>(r_buff.data()),
                                   new_available);
        if (read_bytes <= 0) return "";

        // First two bytes = source address
        uint16_t src_addr = (static_cast<uint16_t>(r_buff[0]) << 8)
                          | static_cast<uint16_t>(r_buff[1]);
        // Third byte = frequency offset
        int freq_mhz = static_cast<int>(r_buff[2]) + start_freq;

        std::cout << "Received message from node address with frequency "
                  << src_addr << "," << freq_mhz << ".125 MHz" << std::endl;

        // Payload = bytes [3 .. (len-2)] (last byte is packet RSSI)
        if (r_buff.size() > 4) {
            std::vector<uint8_t> payload(r_buff.begin() + 3,
                                         r_buff.end() - 1);
            std::string payload_str(payload.begin(), payload.end());
            std::cout << "Message is " << payload_str << std::endl;

            return payload_str;
        }

        // If RSSI mode is on, interpret last byte
        // if (rssi) {
        //     uint8_t packet_rssi = r_buff.back();
        //     std::cout << "The packet RSSI value: -"
        //               << static_cast<int>(256 - packet_rssi)
        //               << " dBm" << std::endl;
        //     get_channel_rssi();
        // }
    }
}

// -------------------- get_channel_rssi() --------------------

void sx126x::get_channel_rssi() {
    // Ensure normal mode: M0=0, M1=0
    gpioWrite(M0, PI_LOW);
    gpioWrite(M1, PI_LOW);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    serialFlush(serial_handle);

    // Send “get RSSI” command (6 bytes)
    uint8_t cmd[6] = { 0xC0, 0xC1, 0xC2, 0xC3, 0x00, 0x02 };
    serWrite(serial_handle, reinterpret_cast<char*>(cmd), 6);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    int available = serDataAvailable(serial_handle);
    if (available > 0) {
        std::vector<uint8_t> re_temp(available, 0);
        serRead(serial_handle,
                   reinterpret_cast<char*>(re_temp.data()),
                   available);

        // Expect at least 4 bytes: [0xC1, 0x00, 0x02, noise_rssi, packet_rssi?]
        if (re_temp.size() >= 4
            && re_temp[0] == 0xC1
            && re_temp[1] == 0x00
            && re_temp[2] == 0x02)
        {
            uint8_t noise_rssi = re_temp[3];
            std::cout << "Current noise RSSI value: -"
                      << static_cast<int>(256 - noise_rssi)
                      << " dBm" << std::endl;
            // re_temp[4] (if present) is the last packet’s RSSI
        }
        else {
            std::cout << "Receive RSSI value fail" << std::endl;
        }
    }
    else {
        std::cout << "No RSSI response received" << std::endl;
    }
}
; // end class sx126x


// Example usage:
// int main() {
//     if (gpioInitialise() < 0) {
//         std::cerr << "pigpio initialization failed" << std::endl;
//         return 1;
//     }
//
//     // Create SX126X instance (e.g., freq=868 MHz, address=0x1234, power=22dBm, rssi enabled)
//     sx126x lora("/dev/ttyS0", 868, 0x1234, 22, true, 2400, 0, 240, 0, false, false, false);
//
//     // Send a test packet (dest addr high, dest addr low, freq offset, payload)
//     std::vector<uint8_t> packet;
//     packet.push_back(0x12);             // dest addr high byte
//     packet.push_back(0x34);             // dest addr low byte
//     packet.push_back(static_cast<uint8_t>(868 - lora.start_freq)); // freq offset
//     std::string msg = "Hello LoRa";
//     packet.insert(packet.end(), msg.begin(), msg.end());
//     lora.send(packet);
//
//     // Periodically call receive()
//     while (true) {
//         lora.receive();
//         std::this_thread::sleep_for(std::chrono::milliseconds(200));
//     }
//
//     gpioTerminate();
//     return 0;
// }
