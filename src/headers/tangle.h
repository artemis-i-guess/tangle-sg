#ifndef TANGLE_H
#define TANGLE_H
#include "transaction.h"
#include <unordered_map>
class Tangle {
public:
    void addTransaction(const Transaction& tx);
    void updateCumulativeWeight(const std::string& transaction_id);
    std::string serialize() const; // Converts the Tangle to a string format
    void updateFromSerialized(const std::string& data); // Updates Tangle from serialized string
    std::unordered_map<std::string, Transaction> transactions;
};
#endif