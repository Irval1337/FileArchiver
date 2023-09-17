#pragma once

#include <cstdint>
#include <fstream>
#include "BitVector.h"
#include <vector>

class FileReader {
public:
	FileReader(std::string file_path) {
		_stream = std::ifstream(file_path, std::fstream::binary);
		_curr_pos = 0;
		_stream.read((char*)&_curr_byte, sizeof(_curr_byte));
	}
	FileReader(const FileReader&) = delete;
	~FileReader() {
		_stream.close();
	}

	bool get_next_bit() {
		bool val = (_curr_byte >> (7 - _curr_pos)) & 1;
		++_curr_pos;
		if (_curr_pos == 8) {
			_curr_pos = 0;
			_stream.read((char*)&_curr_byte, sizeof(_curr_byte));
		}
		return val;
	}

	uint64_t get_from_n_bits(int n) {
		uint64_t val = 0;
		for (int i = 0; i < n; ++i) {
			val |= get_next_bit() * (1LL << (n - i - 1));
		}
		return val;
	}

	BitVector from_gamma_code() {
		BitVector bits;
		int n = 0;
		while (get_next_bit() == 0) {
			++n;
		}
		for (int j = 0; j < n; ++j) {
			bits.push_back(get_next_bit());
		}
		return bits;
	}
private:
	uint8_t _curr_byte;
	int _curr_pos;
	std::ifstream _stream;
};