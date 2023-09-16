#include "TrieNode.h"
#include <iostream>

TrieNode::TrieNode() {
	_curr_byte = 0;
	memset(_next, 0, sizeof(_next));
	_ends = _count = _code = 0;
}

TrieNode::TrieNode(uint8_t byte) {
	_curr_byte = byte;
	memset(_next, 0, sizeof(_next));
	_ends = _count = _code = 0;
}

TrieNode::~TrieNode() {
	for (int i = 0; i < 256; ++i) {
		delete _next[i];
	}
}

uint8_t TrieNode::get_byte() {
	return _curr_byte;
}
TrieNode** TrieNode::get_next() {
	TrieNode** ptr { _next };
	return ptr;
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