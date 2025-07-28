#include "../headers/pow.h"
#include <openssl/sha.h>
#include <sstream>
#include <iostream>

std::string clockwork_pow(const std::string& data, int nonce) {
    // 1. Combine data + nonce
    std::string input = data + std::to_string(nonce);
    std::string hash = input;
    
    // 2. Fixed iterations (e.g., 65,536)
    const int ITERATIONS = 65536; 
    for(int i = 0; i < ITERATIONS; i++) {
        hash = sha256(hash); // Always same work
    }
    return hash;
}

std::string performPoW(const std::string& data, int difficulty) {
    int nonce = 0;
    while(nonce < MAX_ATTEMPTS) {
        // 3. Always full computation
        std::string hash = clockwork_pow(data, nonce); 
        
        // 4. Standard leading-zero check
        if(hash.substr(0, difficulty) == std::string(difficulty, '0')) {
            return hash;
        }
        nonce++;
    }
    return "FAIL";
}