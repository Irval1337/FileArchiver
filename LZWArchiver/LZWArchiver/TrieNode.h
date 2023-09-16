#pragma once

#include <stdint.h>

class TrieNode {
public:
	TrieNode();
	TrieNode(uint8_t);
	~TrieNode();
	uint8_t get_byte();
	TrieNode** get_next();
	int* get_ends();
	int* get_count();
	int* get_code();
private:
	uint8_t _curr_byte;
	int _code;
	TrieNode* _next[256];
	int _ends;
	int _count;
};