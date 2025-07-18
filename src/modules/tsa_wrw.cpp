#include "../headers/tsa.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <random>
#include <cmath>
#include <ctime>

using namespace std;

vector<string> selectTipsMCMC(Tangle& tangle, double alpha = 0.1) {
    // Initialize random engine
    random_device rd;
    mt19937 rng(rd());
    
    // Handle empty tangle
    if (tangle.transactions.empty()) {
        return {};
    }

    // Build children mapping (parent -> direct approvers)
    unordered_map<string, vector<string>> children_map;
    for (const auto& pair : tangle.transactions) {
        const string& child = pair.first;
        for (const string& parent : pair.second.validating_transactions) {
            children_map[parent].push_back(child);
        }
    }

    // Find genesis transaction (no parents)
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

    // Perform two independent MCMC random walks
    vector<string> tips;
    for (int i = 0; i < 2; ++i) {
        string current = genesis;
        
        while (true) {
            auto it = children_map.find(current);
            if (it == children_map.end() || it->second.empty()) {
                break;  // Reached tip
            }
            
            const vector<string>& approvers = it->second;
            
            // Single approver shortcut
            if (approvers.size() == 1) {
                current = approvers[0];
                continue;
            }

            // Calculate weights with numerical stability
            double max_weight = -1e300;
            for (const auto& tx : approvers) {
                double cw = tangle.transactions[tx].cumulative_weight;
                if (cw > max_weight) max_weight = cw;
            }

            vector<double> weights;
            double total_weight = 0.0;
            for (const auto& tx : approvers) {
                double w = exp(alpha * (tangle.transactions[tx].cumulative_weight - max_weight));
                weights.push_back(w);
                total_weight += w;
            }

            // Roulette wheel selection
            uniform_real_distribution<double> dist(0.0, total_weight);
            double r = dist(rng);
            double cumulative = 0.0;
            
            for (size_t j = 0; j < approvers.size(); ++j) {
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