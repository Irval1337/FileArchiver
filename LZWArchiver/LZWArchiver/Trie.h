#pragma once

#include "TrieNode.h"
#include <vector>

class Trie {
public:
	Trie();
	Trie(const Trie&) = delete;
	~Trie();

	void add(std::vector<uint8_t>::iterator, std::vector<uint8_t>::iterator, int, TrieNode*);
	TrieNode* find(std::vector<uint8_t>::iterator, std::vector<uint8_t>::iterator, TrieNode*);
	void clear();
	int get_size();
private:
	TrieNode* _root;
};