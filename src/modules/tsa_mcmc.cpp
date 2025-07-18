#include "../headers/tsa.h"
#include <climits>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <unordered_map>
#include <vector>
#include <string>

using namespace std;

vector<string> selectTips(Tangle& tangle, double alpha = 0.1) {
    // Initialize random seed
    srand(time(nullptr));
    
    // Handle empty tangle case
    if (tangle.transactions.empty()) {
        return {};
    }

    // 1. Build children mapping (transaction -> its direct approvers)
    unordered_map<string, vector<string>> children_map;
    for (const auto& pair : tangle.transactions) {
        const string& child = pair.first;
        for (const string& parent : pair.second.validating_transactions) {
            children_map[parent].push_back(child);
        }
    }

    // 2. Find genesis transaction (no parents)
    string genesis;
    for (const auto& pair : tangle.transactions) {
        if (pair.second.validating_transactions.empty()) {
            genesis = pair.first;
            break;
        }
    }

    // Fallback if no genesis found
    if (genesis.empty()) {
        return {"genesis_fallback"};
    }

    // 3. Perform two independent MCMC random walks
    vector<string> tips;
    for (int i = 0; i < 2; ++i) {  // Always select 2 tips
        string current = genesis;
        
        while (true) {
            // Get direct approvers of current transaction
            auto it = children_map.find(current);
            vector<string> approvers;
            if (it != children_map.end()) {
                approvers = it->second;
            }

            // Terminate walk if no approvers (reached tip)
            if (approvers.empty()) {
                break;
            }

            // 4. Calculate selection probabilities with numerical stability
            double max_weight = -1e300;
            for (const auto& tx : approvers) {
                double cw = tangle.transactions[tx].cumulative_weight;
                if (cw > max_weight) max_weight = cw;
            }

            double total_weight = 0.0;
            vector<double> weights;
            for (const auto& tx : approvers) {
                // Exponential bias toward higher weights
                double w = exp(alpha * (tangle.transactions[tx].cumulative_weight - max_weight));
                weights.push_back(w);
                total_weight += w;
            }

            // 5. Probabilistic selection (roulette wheel)
            double r = static_cast<double>(rand()) / RAND_MAX * total_weight;
            double cumulative = 0.0;
            for (int j = 0; j < approvers.size(); ++j) {
                cumulative += weights[j];
                if (r <= cumulative) {
                    current = approvers[j];
                    break;
                }
            }
        }
        tips.push_back(current);
    }
    return tips;
}