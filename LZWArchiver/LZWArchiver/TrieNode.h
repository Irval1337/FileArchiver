#pragma once

#include <stdint.h>
#include <map>

class TrieNode {
public:
	TrieNode();
	TrieNode(uint8_t);
	~TrieNode();
	uint8_t get_byte();
	std::map<uint8_t, TrieNode*>* get_next();
	int* get_ends();
	int* get_count();
	int* get_code();
private:
	uint8_t _curr_byte;
	int _code;
	std::map<uint8_t, TrieNode*> _next;
	int _ends;
	int _count;
};