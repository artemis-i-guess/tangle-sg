#include "../headers/pow.h"
#include <openssl/sha.h>
#include <sstream>
#include <iostream>
#include <iomanip>  // Required for std::setw and std::setfill


std::string sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)input.c_str(), input.size(), hash);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

// Main PoW function with Counter-Mode chaining
std::string performPoW(const std::string& data, int difficulty) {
    std::cout << "Performing Counter-Mode PoW for: " << data << std::endl;
    int nonce = 0;
    const int MAX_ATTEMPTS = 1e8;

    while (nonce < MAX_ATTEMPTS) {
        std::string chain = data + std::to_string(nonce);
        
        // Counter-mode iterations (fixed 64 rounds)
        for (int i = 0; i < 64; i++) {
            std::string block = chain + "|" + std::to_string(i);
            chain = sha256(block);
        }
        
        // Final hash
        std::string hash = sha256(chain);

        // Difficulty check (leading zeros)
        if (hash.substr(0, difficulty) == std::string(difficulty, '0')) {
            std::cout << "Counter-Mode PoW solved for " << data 
                      << " at nonce: " << nonce << std::endl;
            return hash;
        }
        nonce++;
    }
    
    std::cerr << "Counter-Mode PoW failed: max attempts reached for " 
              << data << std::endl;
    return "INVALID_POW";
}