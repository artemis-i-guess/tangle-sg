#include "../headers/tsa.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <random>
#include <ctime>

using namespace std;

vector<string> selectTipsURW(Tangle& tangle) {
    // Initialize random engine
    random_device rd;
    mt19937 rng(rd()); // the rng is used later to generate random numbers
    
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

    // Perform two independent unweighted random walks
    vector<string> tips;
    for (int i = 0; i < 2; ++i) {
        string current = genesis;
        
        while (true) {
            auto it = children_map.find(current);
            if (it == children_map.end() || it->second.empty()) {
                break;  // Reached tip
            }
            
            // Uniform random selection
            const vector<string>& approvers = it->second;
            uniform_int_distribution<size_t> dist(0, approvers.size() - 1);
            size_t idx = dist(rng); //rng used to generate a random index
            current = approvers[idx];
        }
        tips.push_back(current);
    }
    return tips;
}