#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cstdint>
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

// === Block structure ===
struct Block {
    uint64_t index;
    std::string previousHash;
    std::string data;
    uint64_t timestamp;
    uint64_t nonce;
    std::string hash;

    Block(uint64_t idx, const std::string& prev, const std::string& d)
        : index(idx), previousHash(prev), data(d), timestamp(std::time(nullptr)), nonce(0) {}

    std::string computeHash(uint64_t testNonce) const {
        std::ostringstream ss;
        ss << index << previousHash << data << timestamp << testNonce;
        return sha256_hex(ss.str());
    }

    // Mining (Proof of Work)
    void mineBlock(uint32_t difficulty) {
        std::string prefix(difficulty, '0');
        while (true) {
            hash = computeHash(nonce);
            if (hash.substr(0, difficulty) == prefix)
                break;
            ++nonce;
        }
    }
};

// === Blockchain class ===
class Blockchain {
private:
    std::vector<Block> chain;
    uint32_t difficulty;

public:
    Blockchain(uint32_t diff) : difficulty(diff) {
        chain.emplace_back(0, "0", "Genesis Block");
        chain[0].mineBlock(difficulty);
    }

    void addBlock(const std::string& data) {
        const std::string prevHash = chain.back().hash;
        Block newBlock(chain.size(), prevHash, data);

        auto start = std::chrono::high_resolution_clock::now();
        newBlock.mineBlock(difficulty);
        auto end = std::chrono::high_resolution_clock::now();

        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        chain.push_back(newBlock);

        std::cout << "Block " << newBlock.index << " mined!" << std::endl;
        std::cout << "Hash: " << newBlock.hash.substr(0, 40) << "..." << std::endl;
        std::cout << "Nonce: " << newBlock.nonce << std::endl;
        std::cout << "Time: " << elapsed << " ms" << std::endl << std::endl;
    }
};

int main() {
    std::vector<int> difficulties = {2, 3, 4, 5}; // Adjust if slow

    for (int diff : difficulties) {
        std::cout << "==============================" << std::endl;
        std::cout << "Mining with difficulty: " << diff << std::endl;
        std::cout << "==============================" << std::endl;

        Blockchain bc(diff);
        bc.addBlock("Transaction 1");
        bc.addBlock("Transaction 2");
        bc.addBlock("Transaction 3");
    }

    return 0;
}
