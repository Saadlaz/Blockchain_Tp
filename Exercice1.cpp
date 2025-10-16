#include <iostream>
#include <vector>
#include <string>
#include <openssl/sha.h>

using namespace std;

// Function to compute SHA256 hash of a string
string computeSHA256(const string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)data.c_str(), data.size(), hash);

    char hexStr[2 * SHA256_DIGEST_LENGTH + 1];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hexStr + (i * 2), "%02x", hash[i]);
    }
    hexStr[2 * SHA256_DIGEST_LENGTH] = 0;

    return string(hexStr);
}

// Class representing a Merkle Tree
class MerkleTree {
private:
    vector<string> leaves;
    vector<string> tree;

    // Build the Merkle Tree
    void buildTree() {
        tree.clear();
        for (const auto& leaf : leaves) {
            tree.push_back(computeSHA256(leaf));
        }

        int offset = 0;
        while (tree.size() - offset > 1) {
            int levelSize = tree.size() - offset;
            for (int i = 0; i < levelSize; i += 2) {
                if (i + 1 < levelSize) {
                    string combined = tree[offset + i] + tree[offset + i + 1];
                    tree.push_back(computeSHA256(combined));
                } else {
                    tree.push_back(tree[offset + i]); // Handle odd number of nodes
                }
            }
            offset += levelSize;
        }
    }

public:
    MerkleTree(const vector<string>& data) : leaves(data) {
        buildTree();
    }

    // Get the root of the Merkle Tree
    string getRoot() const {
        return tree.empty() ? "" : tree.back();
    }

    // Print the Merkle Tree
    void printTree() const {
        int level = 0;
        int count = 1;
        for (size_t i = 0; i < tree.size(); i++) {
            cout << "Level " << level << ": " << tree[i] << endl;
            if (--count == 0) {
                level++;
                count = 1 << level;
            }
        }
    }
};

int main() {
    vector<string> data1 = {"A", "B", "C", "D"};
    MerkleTree tree1(data1);
    cout << "Example 1:\n";
    cout << "Merkle Root: " << tree1.getRoot() << "\n";
    tree1.printTree();

    cout << "\n---------------------------------------------\n";

    vector<string> data2 = {"Alice pays Bob", "Bob pays Charlie", "Charlie pays Dave"};
    MerkleTree tree2(data2);
    cout << "Example 2:\n";
    cout << "Merkle Root: " << tree2.getRoot() << "\n";
    tree2.printTree();

    return 0;
}
