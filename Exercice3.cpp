// Exercice3.cpp: Proof of Stake Implementation with Comparison to Proof of Work

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cstdint>
#include <random>  // For random selection in PoS
#include <openssl/sha.h>

// === Helper function: compute SHA256 and return hex string ===
std::string sha256_hex(const std::string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), hash);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        oss << std::setw(2) << static_cast<int>(hash[i]);
    return oss.str();
}

// === Block structure (shared for PoW and PoS) ===
struct Block {
    uint64_t index;
    std::string previousHash;
    std::string data;
    uint64_t timestamp;
    uint64_t nonce;  // Used in PoW; in PoS, can be validator ID or similar
    std::string hash;

    Block(uint64_t idx, const std::string& prev, const std::string& d)
        : index(idx), previousHash(prev), data(d), timestamp(std::time(nullptr)), nonce(0) {}

    std::string computeHash(uint64_t testNonce) const {
        std::ostringstream ss;
        ss << index << previousHash << data << timestamp << testNonce;
        return sha256_hex(ss.str());
    }
};

// === Proof of Work Blockchain ===
class PoWBlockchain {
private:
    std::vector<Block> chain;
    uint32_t difficulty;

public:
    PoWBlockchain(uint32_t diff) : difficulty(diff) {
        chain.emplace_back(0, "0", "Genesis Block");
        mineBlock(chain.back());
    }

    void addBlock(const std::string& data) {
        const std::string prevHash = chain.back().hash;
        Block newBlock(chain.size(), prevHash, data);
        mineBlock(newBlock);
        chain.push_back(newBlock);
    }

    void mineBlock(Block& block) {
        std::string prefix(difficulty, '0');
        while (true) {
            block.hash = block.computeHash(block.nonce);
            if (block.hash.substr(0, difficulty) == prefix)
                break;
            ++block.nonce;
        }
    }

    size_t size() const { return chain.size(); }
};

// === Proof of Stake Blockchain ===
class PoSBlockchain {
private:
    std::vector<Block> chain;
    std::vector<std::pair<std::string, uint64_t>> validators;  // Validator name and stake

    // Select validator based on stake (weighted random)
    std::string selectValidator() {
        uint64_t totalStake = 0;
        for (const auto& v : validators) {
            totalStake += v.second;
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> dis(0, totalStake - 1);

        uint64_t rand = dis(gen);
        uint64_t current = 0;
        for (const auto& v : validators) {
            current += v.second;
            if (rand < current) {
                return v.first;
            }
        }
        return validators.back().first;  // Fallback
    }

public:
    PoSBlockchain(const std::vector<std::pair<std::string, uint64_t>>& vals) : validators(vals) {
        chain.emplace_back(0, "0", "Genesis Block");
        forgeBlock(chain.back());
    }

    void addBlock(const std::string& data) {
        const std::string prevHash = chain.back().hash;
        Block newBlock(chain.size(), prevHash, data);
        forgeBlock(newBlock);
        chain.push_back(newBlock);
    }

    void forgeBlock(Block& block) {
        // In PoS, "forging" is quick: select validator and compute hash once
        std::string validator = selectValidator();
        block.nonce = 0;  // Nonce not used for puzzle, could store validator ID
        block.data += " (Forged by: " + validator + ")";  // Simulate validator signature
        block.hash = block.computeHash(block.nonce);
    }

    size_t size() const { return chain.size(); }
};

int main() {
    // Define difficulties for PoW and number of blocks to add
    std::vector<int> difficulties = {2, 3, 4};  // Lower difficulties for faster testing; adjust as needed
    int numBlocks = 5;  // Number of blocks to add for timing

    // Define validators for PoS (name, stake)
    std::vector<std::pair<std::string, uint64_t>> validators = {
        {"Validator1", 100},
        {"Validator2", 200},
        {"Validator3", 150}
    };

    for (int diff : difficulties) {
        std::cout << "==============================" << std::endl;
        std::cout << "Testing with difficulty/PoS equivalent: " << diff << std::endl;
        std::cout << "==============================" << std::endl;

        // === Proof of Work Timing ===
        auto powStart = std::chrono::high_resolution_clock::now();
        PoWBlockchain powBC(diff);
        for (int i = 1; i <= numBlocks; ++i) {
            powBC.addBlock("Transaction " + std::to_string(i));
            std::cout << "PoW Block " << powBC.size() - 1 << " added." << std::endl;
        }
        auto powEnd = std::chrono::high_resolution_clock::now();
        long long powTime = std::chrono::duration_cast<std::chrono::milliseconds>(powEnd - powStart).count();

        std::cout << "PoW Total Time for " << numBlocks << " blocks: " << powTime << " ms" << std::endl << std::endl;

        // === Proof of Stake Timing ===
        auto posStart = std::chrono::high_resolution_clock::now();
        PoSBlockchain posBC(validators);
        for (int i = 1; i <= numBlocks; ++i) {
            posBC.addBlock("Transaction " + std::to_string(i));
            std::cout << "PoS Block " << posBC.size() - 1 << " added." << std::endl;
        }
        auto posEnd = std::chrono::high_resolution_clock::now();
        long long posTime = std::chrono::duration_cast<std::chrono::milliseconds>(posEnd - posStart).count();

        std::cout << "PoS Total Time for " << numBlocks << " blocks: " << posTime << " ms" << std::endl << std::endl;

        // Comparison
        std::cout << "Comparison: PoS is " << (powTime > posTime ? "faster" : "slower") << " than PoW by "
                  << std::abs(powTime - posTime) << " ms." << std::endl << std::endl;
    }

    return 0;
}