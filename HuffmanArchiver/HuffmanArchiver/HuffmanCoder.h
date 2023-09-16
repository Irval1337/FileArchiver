#pragma once

#include <cstdint>
#include <vector>
#include <set>
#include "BitVector.h"

class HuffmanCoder {
public:
	HuffmanCoder() = default;
	void generate_table(std::vector<uint8_t>&);
	std::vector<std::pair<uint8_t, BitVector>> get_table();
private:
	struct Node {
		Node* L, * R;
		uint8_t val;
		Node() {
			L = R = nullptr;
			val = 0;
		}
		Node(uint8_t val) {
			L = R = nullptr;
			this->val = val;
		}
		bool is_leaf() {
			return L == R && R == nullptr;
		}
	};

	Node* generate_tree(std::vector<uint8_t>&);
	std::vector<BitVector> generate_table(Node*);

	std::vector<std::pair<uint8_t, BitVector>> _table;
};