#pragma once

#include <vector>

class BitVector {
private:
	std::vector<uint64_t> _value;
	uint64_t _sz;
public:
	BitVector() {
		_value.push_back(0);
		_sz = 0;
	}
	uint64_t size() {
		return _sz;
	}
	void push_back(bool val) {
		if (_sz % 64 == 0)
			_value.push_back(0);
		int j = _sz / 64;
		_value[j] |= (1LL << (63 - (_sz - j * 64))) * val;
		++_sz;
	}
	bool empty() {
		return _sz == 0;
	}
	std::vector<uint64_t> value() {
		return _value;
	}

	bool operator[](uint64_t ind) {
		int j = ind / 64;
		return (_value[j] >> (63 - (ind - 64 * j))) & 1;
	}
};