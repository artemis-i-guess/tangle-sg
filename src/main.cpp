#include <iostream>
#include <vector>
#include <unordered_map>
#include <random>
#include <thread>
#include <chrono>
#include "headers/pow.h"
#include "headers/tsa.h"
#include "headers/transaction.h"
#include "headers/tangle.h"
#include "headers/network.h"

using namespace std;

Transaction simulateSmartMeter(Tangle& tangle) {
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> energyDist(0.5, 5.0);
    uniform_real_distribution<> priceDist(0.1, 0.5);

    while (true) {
        vector<string> parents = selectTips(tangle);

        Transaction newTx;
        newTx.transaction_id = "tx" + to_string(rand());
        newTx.timestamp = to_string(time(nullptr));
        newTx.sender = "Meter_001";
        newTx.receiver = "Grid";
        newTx.amount = energyDist(gen);
        newTx.unit = "kWh";
        newTx.price_per_unit = priceDist(gen);
        newTx.currency = "USD";
        newTx.previous_transactions = parents;
        newTx.proof_of_work = "Pending";

        cout << "[LOG] Generating new transaction: " << newTx.transaction_id << endl;
        
         // Compute PoW for new transaction
        newTx.proof_of_work = performPoW(newTx.transaction_id, 2);

        // Update cumulative weight for selected tips
        for (const string& parent : parents) {
            tangle.updateCumulativeWeight(parent);
        }

        // Add the new transaction
        tangle.addTransaction(newTx);
        
        cout << "[LOG] Transaction " << newTx.transaction_id << " added to Tangle." << endl;
        broadcastTangle(tangle);
        this_thread::sleep_for(chrono::minutes(5));
    }
}

int main() {
    Tangle tangle;

    // Create genesis transaction (without PoW initially)
    Transaction genesis = {"tx0", "2025-03-11T12:00:00Z", "node_A", "node_B", 5.0, "kWh", 0.12, "USD", {}, {"tx3", "tx4"}, 1, ""};

    // Compute PoW separately
    genesis.proof_of_work = performPoW(genesis.transaction_id, 3);
    tangle.addTransaction(genesis);

    thread serverThread(startServer, ref(tangle));

    // Start transaction simulation in a separate thread
    thread simulationThread(simulateSmartMeter, ref(tangle));

    // Join the threads to keep the main function active
    serverThread.join();
    simulationThread.join();

    return 0;
}
