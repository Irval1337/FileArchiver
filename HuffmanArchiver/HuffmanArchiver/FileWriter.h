#pragma once

#include <cstdint>
#include <fstream>
#include "BitVector.h"

class FileWriter {
public:
	FileWriter(std::string file_path) {
		_stream = std::ofstream(file_path, std::fstream::trunc | std::fstream::binary);
		_curr_byte = _curr_pos = 0;
	}
	FileWriter(const FileWriter&) = delete;
	~FileWriter() {
		if (_curr_pos != 0) {
			_stream.write((char*)&_curr_byte, sizeof(uint8_t));
		}
		_stream.close();
	}

	std::pair<BitVector, BitVector> to_gamma_code(BitVector& bits) {
		BitVector added1 = bits, added2;
		added2.push_front(1);
		int sz = (int)added1.size();
		for (int i = 0; i < sz; ++i) {
			added2.push_front(0);
		}
		return { added2, added1 };
	}

	void write_bits(BitVector& bits) {
		int written = 0;
		while (bits.size() != written) {
			while (_curr_pos < 8 && bits.size() != written) {
				if (bits[written] == 1)
					_curr_byte |= 1LL << (7 - _curr_pos);
				++_curr_pos;
				++written;
			}
			if (_curr_pos == 8) {
				_stream.write((char*)&_curr_byte, sizeof(uint8_t));
				_curr_pos = 0;
				_curr_byte = 0;
			} else break;
		}
	}

	BitVector get_bits(uint64_t n, int bits_count) {
		BitVector vec;
		for (int i = 0; i < bits_count; ++i) {
			vec.push_back((n >> (bits_count - 1 - i)) & 1);
		}
		return vec;
	}

private:
	uint8_t _curr_byte;
	int _curr_pos;
	std::ofstream _stream;
};