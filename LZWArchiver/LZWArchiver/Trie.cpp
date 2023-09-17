#include "Trie.h"

Trie::Trie() {
	_root = new TrieNode(0);
}

Trie::~Trie() {
	delete _root;
}

void Trie::add(std::vector<uint8_t>::iterator itBeg, std::vector<uint8_t>::iterator itEnd, int code, TrieNode* start) {
	if (start == nullptr)
		start = _root;

	if (itBeg == itEnd) {
		*start->get_ends() += 1;
		*start->get_code() = code;
		return;
	}

	*start->get_count() += 1;
	uint8_t curr_byte = *itBeg;

	if (start->get_next()->count(curr_byte) == 0)
		start->get_next()->insert(std::make_pair(curr_byte, new TrieNode(curr_byte)));

	auto tmpIt = itBeg;
	tmpIt++;

	add(tmpIt, itEnd, code, start->get_next()->operator[](curr_byte));
}

TrieNode* Trie::find(std::vector<uint8_t>::iterator itBeg, std::vector<uint8_t>::iterator itEnd, TrieNode* start) {
	if (start == nullptr)
		start = _root;

	if (itBeg == itEnd)
		return *start->get_ends() == 0 ? nullptr : start;

	uint8_t curr_byte = *itBeg;
	if (start->get_next()->count(curr_byte) == 0) return nullptr;

	auto tmpIt = itBeg;
	tmpIt++;
	return find(tmpIt, itEnd, start->get_next()->operator[](curr_byte));
}

void Trie::clear() {
	delete _root;
	_root = new TrieNode(0);
}

int Trie::get_size() {
	return *_root->get_count();
}