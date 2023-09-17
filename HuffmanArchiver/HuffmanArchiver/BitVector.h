#pragma once

class BitVector {
private:
	uint64_t _value;
	uint64_t _sz;
public:
	BitVector() {
		_value = 0;
		_sz = 0;
	}
	uint64_t size() {
		return _sz;
	}
	void push_back(bool val) {
		_value |= (1LL << (63 - _sz)) * val;
		++_sz;
	}
	void pop_back() {
		if (this->operator[](_sz - 1))
			_value ^= 1LL << (64 - _sz);
		--_sz;
	}
	void push_front(bool val) {
		_value = (_value >> 1) | ((1LL << 63) * val);
		++_sz;
	}
	void set(uint64_t ind, bool val) {
		if (this->operator[](ind))
			_value ^= 1LL << (63 - ind);
		_value |= (1LL << (63 - ind)) * val;
	}
	bool empty() {
		return _sz == 0;
	}
	uint64_t value() {
		return _value;
	}

	bool operator[](uint64_t ind) {
		return (_value >> (63 - ind)) & 1;
	}
};