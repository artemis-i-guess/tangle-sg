#ifndef TRANSACTION_H
#define TRANSACTION_H
#include <string>
#include <vector>
struct Transaction {
    std::string transaction_id;
    std::string timestamp;
    int timestampInt;
    std::string sender;
    std::string receiver;
    double amount;
    std::string unit;
    double price_per_unit;
    std::string currency;
    std::vector<std::string> previous_transactions;
    std::vector<std::string> validating_transactions;
    int cumulative_weight;
    std::string proof_of_work;
};
#endif