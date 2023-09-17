#include "HuffmanCoder.h"

HuffmanCoder::Node* HuffmanCoder::generate_tree(std::vector<uint8_t>& file_buff) {
	std::vector<int> freq(256, 0);
	for (uint64_t i = 0; i < file_buff.size(); ++i) {
		++freq[file_buff[i]];
	}

	std::set<std::pair<std::pair<int, int>, Node*>> prior;
	for (int i = 0; i < 256; ++i) {
		if (freq[i] != 0)
			prior.insert(std::make_pair(std::make_pair(freq[i], -i), new Node(i)));
	}

	int _time = 256;
	while (prior.size() > 1) {
		std::pair<std::pair<int, int>, Node*> top1 = *prior.begin();
		prior.erase(prior.begin());
		std::pair<std::pair<int, int>, Node*> top2 = *prior.begin();
		prior.erase(prior.begin());

		Node* new_node = new Node();
		new_node->L = top1.second;
		new_node->R = top2.second;
		prior.insert(std::make_pair(std::make_pair(top1.first.first + top2.first.first, -_time), new_node));
		++_time;
	}
	return prior.begin()->second;
}

std::vector<BitVector> HuffmanCoder::generate_table(HuffmanCoder::Node* p) {
	std::vector<BitVector> table(256);
	BitVector curr;
	auto dfs = [&](Node* p, auto f) {
		if (p->is_leaf()) {
			table[p->val] = curr;
			return;
		}
		if (p->L != nullptr) {
			curr.push_back(0);
			f(p->L, f);
			curr.pop_back();
		}
		if (p->R != nullptr) {
			curr.push_back(1);
			f(p->R, f);
			curr.pop_back();
		}
	};

	dfs(p, dfs);
	return table;
}

void HuffmanCoder::generate_table(std::vector<uint8_t>& file_buff) {
	Node* p = generate_tree(file_buff);
	std::vector<BitVector> table = generate_table(p);
	std::vector<std::pair<uint8_t, BitVector>> filtered;
	for (int i = 0; i < 256; ++i) {
		if (table[i].empty()) continue;
		filtered.push_back(std::make_pair(i, table[i]));
	}
	_table = filtered;
}

std::vector<std::pair<uint8_t, BitVector>> HuffmanCoder::get_table() {
	return _table;
}