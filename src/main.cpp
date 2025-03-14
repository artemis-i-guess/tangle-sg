#include <iostream>
#include <vector>
#include <unordered_map>
#include <random>
#include <chrono>
#include "headers/pow.h"
#include "headers/tsa.h"
#include "headers/transaction.h"
#include "headers/tangle.h"

using namespace std;

int main() {
    Tangle tangle;

    // Create genesis transaction (without PoW initially)
    Transaction genesis = {"tx0", "2025-03-11T12:00:00Z", "node_A", "node_B", 5.0, "kWh", 0.12, "USD", {}, {"tx3", "tx4"}, 1, ""};

    // Compute PoW separately
    genesis.proof_of_work = performPoW(genesis.transaction_id, 3);
    tangle.addTransaction(genesis);

    // Select tips
    vector<string> parents = selectTips(tangle);

    // Create a new transaction (without PoW initially)
    Transaction newTx = {"tx1", "2025-03-11T12:10:00Z", "node_C", "node_D", 3.0, "kWh", 0.15, "USD", parents, {}, 1, ""};

    // Compute PoW for new transaction
    newTx.proof_of_work = performPoW(newTx.transaction_id, 3);

    // Update cumulative weight for selected tips
    for (const string& parent : parents) {
        tangle.updateCumulativeWeight(parent);
    }

    // Add the new transaction
    tangle.addTransaction(newTx);

    cout << "New transaction added: " << newTx.transaction_id << " with PoW: " << newTx.proof_of_work << "\n";
    return 0;
}
