#ifndef TANGLE_H
#define TANGLE_H
#include "transaction.h"
#include <unordered_map>
class Tangle {
public:
    void addTransaction(const Transaction& tx);
    void updateCumulativeWeight(const std::string& transaction_id);
    std::unordered_map<std::string, Transaction> transactions;
};
#endif