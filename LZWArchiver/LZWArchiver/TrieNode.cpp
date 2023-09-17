#include "TrieNode.h"
#include <iostream>

TrieNode::TrieNode() {
	_curr_byte = 0;
	_ends = _count = _code = 0;
}

TrieNode::TrieNode(uint8_t byte) {
	_curr_byte = byte;
	_ends = _count = _code = 0;
}

TrieNode::~TrieNode() {
	for (auto& u : _next) {
		if (u.second != nullptr)
			delete u.second;
	}
}

uint8_t TrieNode::get_byte() {
	return _curr_byte;
}
std::map<uint8_t, TrieNode*>* TrieNode::get_next() {
	return &_next;
}
int* TrieNode::get_ends() {
	return &_ends;
}
int* TrieNode::get_count() {
	return &_count;
}
int* TrieNode::get_code() {
	return &_code;
}