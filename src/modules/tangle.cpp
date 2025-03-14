#include "../headers/tangle.h"

void Tangle::addTransaction(const Transaction& tx) {
    transactions[tx.transaction_id] = tx;
}
void Tangle::updateCumulativeWeight(const std::string& transaction_id) {
    transactions[transaction_id].cumulative_weight++;
}