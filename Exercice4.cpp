// Exercice4.cpp: Mini-Blockchain with PoW and PoS Integration

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <cstdint>
#include <random>
#include <openssl/sha.h>

using namespace std;

// === Helper function: compute SHA256 and return hex string ===
string sha256_hex(const string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), hash);
    ostringstream oss;
    oss << hex << setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        oss << setw(2) << static_cast<int>(hash[i]);
    return oss.str();
}

// === Transaction Class ===
class Transaction {
public:
    string id;
    string sender;
    string receiver;
    double amount;

    Transaction(const string& _id, const string& _sender, const string& _receiver, double _amount)
        : id(_id), sender(_sender), receiver(_receiver), amount(_amount) {}

    // Serialize to string for hashing
    string toString() const {
        ostringstream ss;
        ss << id << sender << receiver << amount;
        return ss.str();
    }
};

// === Merkle Tree Class (from Exercice1) ===
class MerkleTree {
private:
    vector<string> leaves;
    vector<string> tree;

    void buildTree() {
        tree.clear();
        for (const auto& leaf : leaves) {
            tree.push_back(sha256_hex(leaf));
        }

        size_t offset = 0;
        while (tree.size() - offset > 1) {
            size_t levelSize = tree.size() - offset;
            for (size_t i = 0; i < levelSize; i += 2) {
                if (i + 1 < levelSize) {
                    string combined = tree[offset + i] + tree[offset + i + 1];
                    tree.push_back(sha256_hex(combined));
                } else {
                    tree.push_back(tree[offset + i]); // Odd number of nodes
                }
            }
            offset += levelSize;
        }
    }

public:
    MerkleTree(const vector<string>& data) : leaves(data) {
        buildTree();
    }

    string getRoot() const {
        return tree.empty() ? "" : tree.back();
    }
};

// === Block Class ===
class Block {
public:
    uint64_t index;
    string previousHash;
    string merkleRoot;
    vector<Transaction> transactions;
    uint64_t timestamp;
    uint64_t nonce;
    string hash;

    Block(uint64_t idx, const string& prev, const vector<Transaction>& txs)
        : index(idx), previousHash(prev), transactions(txs), timestamp(time(nullptr)), nonce(0) {
        // Compute Merkle Root
        vector<string> txStrings;
        for (const auto& tx : transactions) {
            txStrings.push_back(tx.toString());
        }
        MerkleTree mt(txStrings);
        merkleRoot = mt.getRoot();
    }

    // Compute hash of the block with optional validator
    string computeHash(uint64_t testNonce, const string& validator = "") const {
        ostringstream ss;
        ss << index << previousHash << merkleRoot << timestamp << testNonce;
        if (!validator.empty()) {
            ss << " (Validated by: " << validator << ")";
        }
        return sha256_hex(ss.str());
    }

    // Mine for PoW
    void mineBlock(uint32_t difficulty) {
        string prefix(difficulty, '0');
        while (true) {
            hash = computeHash(nonce);
            if (hash.substr(0, difficulty) == prefix) {
                break;
            }
            ++nonce;
        }
    }

    // Forge for PoS
    void forgeBlock(const string& validator) {
        nonce = 0; // Not used for puzzle
        hash = computeHash(nonce, validator);
    }
};

// === Base Blockchain Class ===
class Blockchain {
protected:
    vector<Block> chain;

public:
    Blockchain() {
        // Genesis block
        vector<Transaction> genesisTx = {Transaction("0", "Genesis", "Genesis", 0.0)};
        Block genesis(0, "0", genesisTx);
        chain.push_back(genesis);
    }

    const Block& getLastBlock() const {
        return chain.back();
    }

    // Verify chain integrity
    bool isValid() const {
        for (size_t i = 1; i < chain.size(); ++i) {
            const Block& current = chain[i];
            const Block& previous = chain[i - 1];

            // Check hash link
            if (current.previousHash != previous.hash) {
                return false;
            }

            // Recompute hash (check if PoS validator is needed)
            string recomputedHash = current.computeHash(current.nonce);
            if (current.hash.find("(Validated by:") != string::npos) {
                // For PoS, extract validator from hash
                size_t pos = current.hash.find("(Validated by:");
                if (pos != string::npos) {
                    string validator = current.hash.substr(pos + 13); // Length of "(Validated by: "
                    validator = validator.substr(0, validator.find(")"));
                    recomputedHash = current.computeHash(current.nonce, validator);
                }
            }
            if (current.hash != recomputedHash) {
                return false;
            }
        }
        return true;
    }

    void printChain() const {
        for (const auto& block : chain) {
            cout << "Block " << block.index << ":" << endl;
            cout << "  Prev Hash: " << block.previousHash.substr(0, 10) << "..." << endl;
            cout << "  Merkle Root: " << block.merkleRoot.substr(0, 10) << "..." << endl;
            cout << "  Hash: " << block.hash.substr(0, 10) << "..." << endl;
            cout << "  Transactions: " << block.transactions.size() << endl;
            cout << endl;
        }
    }
};

// === PoW Blockchain ===
class PoWBlockchain : public Blockchain {
private:
    uint32_t difficulty;

public:
    PoWBlockchain(uint32_t diff) : difficulty(diff) {
        chain[0].mineBlock(difficulty); // Mine genesis
    }

    void addBlock(const vector<Transaction>& txs) {
        Block newBlock(chain.size(), getLastBlock().hash, txs);
        newBlock.mineBlock(difficulty);
        chain.push_back(newBlock);
    }
};

// === PoS Blockchain ===
class PoSBlockchain : public Blockchain {
private:
    vector<pair<string, uint64_t>> validators; // Validator name, stake

    string selectValidator() {
        uint64_t totalStake = 0;
        for (const auto& v : validators) {
            totalStake += v.second;
        }

        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<uint64_t> dis(0, totalStake - 1);

        uint64_t randVal = dis(gen);
        uint64_t current = 0;
        for (const auto& v : validators) {
            current += v.second;
            if (randVal < current) {
                return v.first;
            }
        }
        return validators.back().first;
    }

public:
    PoSBlockchain(const vector<pair<string, uint64_t>>& vals) : validators(vals) {
        string validator = selectValidator();
        chain[0].forgeBlock(validator); // Forge genesis
    }

    void addBlock(const vector<Transaction>& txs) {
        Block newBlock(chain.size(), getLastBlock().hash, txs);
        string validator = selectValidator();
        newBlock.forgeBlock(validator);
        chain.push_back(newBlock);
    }
};

int main() {
    // Parameters
    vector<int> difficulties = {2, 3, 4};
    int numBlocks = 5;
    vector<pair<string, uint64_t>> validators = {
        {"Validator1", 100}, {"Validator2", 200}, {"Validator3", 150}
    };

    for (int diff : difficulties) {
        cout << "==============================" << endl;
        cout << "Difficulty/Stake Level: " << diff << endl;
        cout << "==============================" << endl;

        // === PoW Demo ===
        auto powStart = chrono::high_resolution_clock::now();
        PoWBlockchain powChain(diff);
        for (int i = 1; i <= numBlocks; ++i) {
            vector<Transaction> txs = {
                Transaction(to_string(i*10+1), "ILIAS", "mostapha ", 10.0),
                Transaction(to_string(i*10+2), "Nada", "Saad", 5.0)
            };
            powChain.addBlock(txs);
        }
        auto powEnd = chrono::high_resolution_clock::now();
        long long powTime = chrono::duration_cast<chrono::milliseconds>(powEnd - powStart).count();

        cout << "PoW Chain:" << endl;
        powChain.printChain();
        cout << "PoW Valid: " << (powChain.isValid() ? "Yes" : "No") << endl;
        cout << "PoW Time for " << numBlocks << " blocks: " << powTime << " ms" << endl << endl;

        // === PoS Demo ===
        auto posStart = chrono::high_resolution_clock::now();
        PoSBlockchain posChain(validators);
        for (int i = 1; i <= numBlocks; ++i) {
            vector<Transaction> txs = {
                Transaction(to_string(i*10+1), "Alice", "Bob", 10.0),
                Transaction(to_string(i*10+2), "Bob", "Charlie", 5.0)
            };
            posChain.addBlock(txs);
        }
        auto posEnd = chrono::high_resolution_clock::now();
        long long posTime = chrono::duration_cast<chrono::milliseconds>(posEnd - posStart).count();

        cout << "PoS Chain:" << endl;
        posChain.printChain();
        cout << "PoS Valid: " << (posChain.isValid() ? "Yes" : "No") << endl;
        cout << "PoS Time for " << numBlocks << " blocks: " << posTime << " ms" << endl << endl;

        // === Comparison ===
        cout << "Comparison:" << endl;
        cout << "  - Speed: PoS is faster by " << (powTime - posTime) << " ms." << endl;
        cout << "  - Resource Consumption: PoW uses more CPU due to mining loop; PoS is lightweight." << endl;
        cout << "  - Ease of Implementation: PoS is simpler (no intensive computation), but requires validator management." << endl << endl;
    }

    return 0;
}