#include "../headers/tsa.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <random>
#include <numeric>
#include <ctime>

using namespace std;

vector<string> selectTipsGreedyWeightedWalk(Tangle& tangle) {
    // Initialize random number generator
    random_device rd;
    mt19937 rng(rd());
    
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
    if (genesis.empty()) {
        return {"genesis_fallback"};
    }

    // 3. Perform two independent weighted random walks
    vector<string> tips;
    for (int i = 0; i < 2; ++i) {
        string current = genesis;
        
        while (true) {
            auto it = children_map.find(current);
            if (it == children_map.end() || it->second.empty()) {
                break;  // Reached a tip
            }
            
            const vector<string>& approvers = it->second;
            
            // Special case: single approver
            if (approvers.size() == 1) {
                current = approvers[0];
                continue;
            }

            // Calculate total cumulative weight
            double total_weight = 0.0;
            for (const auto& tx : approvers) {
                total_weight += tangle.transactions[tx].cumulative_weight;
            }

            // Generate random number in [0, total_weight)
            uniform_real_distribution<double> dist(0.0, total_weight);
            double r = dist(rng);
            
            // Select approver proportional to its weight
            double cumulative = 0.0;
            for (const auto& tx : approvers) {
                cumulative += tangle.transactions[tx].cumulative_weight;
                if (r <= cumulative) {
                    current = tx;
                    break;
                }
            }
        }
        tips.push_back(current);
    }
    return tips;
}