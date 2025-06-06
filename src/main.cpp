#include <iostream>
#include <vector>
#include <unordered_map>
#include <random>
#include <thread>
#include <chrono>
#include <Python.h>
#include <pigpio.h>
#include <sx126x.h>
#include "headers/pow.h"
#include "headers/tsa.h"
#include "headers/transaction.h"
#include "headers/tangle.h"
#include "headers/network.h"

#include <vector>

using namespace std;
using namespace chrono;

void simulateSmartMeter(Tangle& tangle) {
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> energyDist(0.5, 5.0);
    uniform_real_distribution<> priceDist(0.1, 0.5);

    vector<int> timearray ;
    int i = 0;
    while (i<1000){
        i++;
        vector<string> parents = selectTips(tangle);

        Transaction newTx;
        newTx.transaction_id = "tx" + to_string(rand());
        newTx.timestamp = to_string(time(nullptr));
        newTx.timestampInt = static_cast<int>(time(nullptr));
        newTx.sender = "Meter_001";
        newTx.receiver = "Grid";
        newTx.amount = energyDist(gen);
        newTx.unit = "kWh";
        newTx.price_per_unit = priceDist(gen);
        newTx.currency = "USD";
        newTx.previous_transactions = parents;
        newTx.proof_of_work = "Pending";

        cout << "[LOG] Generating new transaction: " << newTx.transaction_id << " at:" << newTx.timestamp << endl;
        auto start = chrono::high_resolution_clock::now();
         // Compute PoW for new transaction
        newTx.proof_of_work = performPoW(newTx.transaction_id, 2);

        // Update cumulative weight for selected tips
        for (const string& parent : parents) {
            tangle.updateCumulativeWeight(parent);
        }

        // Add the new transaction
        tangle.addTransaction(newTx);
        auto end=chrono::high_resolution_clock::now();
        auto elapsed = duration<double, milli> (end - start).count();
        
        cout << "[LOG] Transaction " << newTx.transaction_id << " added to Tangle." << endl;
        cout << "Time elapsed:" << elapsed << " ms" << endl;

        broadcastTangle(tangle);
        this_thread::sleep_for(chrono::seconds(10));
    }
}

int main() {

    if (gpioInitialise() < 0) {
        std::cerr << "pigpio initialization failed" << std::endl;
        return 1;
    }

    // Create SX126X instance (e.g., freq=868 MHz, address=0x1234, power=22dBm, rssi enabled)
    sx126x lora("/dev/ttyS0", 868, 0x1234, 22, true, 2400, 0, 240, 0, false, false, false);

    // Send a test packet (dest addr high, dest addr low, freq offset, payload)
    std::vector<uint8_t> packet;
    packet.push_back(0x12);             // dest addr high byte
    packet.push_back(0x34);             // dest addr low byte
    packet.push_back(static_cast<uint8_t>(868 - lora.start_freq)); // freq offset
    std::string msg = "Hello LoRa";
    packet.insert(packet.end(), msg.begin(), msg.end());
    lora.send(packet);

    // Periodically call receive()
    while (true) {
        lora.receive();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    gpioTerminate();

    Tangle tangle;

    // Create genesis transaction (without PoW initially)
    Transaction genesis = {"tx0", "2025-03-11T12:00:00Z", 00000000011, "node_A", "node_B", 5.0, "kWh", 0.12, "USD", {}, {"tx3", "tx4"}, 1, ""};

    // Compute PoW separately
    genesis.proof_of_work = performPoW(genesis.transaction_id, 2);
    tangle.addTransaction(genesis);

    thread serverThread(startServer, ref(tangle));
    // thread loraThread(handleLoRaClient, ref(tangle));
    // Start transaction simulation in a separate thread
    // thread simulationThread(simulateSmartMeter, ref(tangle));

    // Join the threads to keep the main function active
    serverThread.join();
    // loraThread.join();
    // simulationThread.join();
    

    return 0;
}
