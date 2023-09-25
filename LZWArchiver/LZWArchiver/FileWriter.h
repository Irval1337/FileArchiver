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

	BitVector to_gamma_code(BitVector& bits) {
		BitVector added;
		int sz = (int)bits.size();
		for (int i = 0; i < sz; ++i) {
			added.push_back(0);
		}
		added.push_back(1);
		for (int i = 0; i < bits.size(); ++i) {
			added.push_back(bits[i]);
		}
		
		return added;
	}

	BitVector to_delta_code(uint64_t x) {
		BitVector x_bits, len_bits;
		bool got = false;
		for (int i = 63; i >= 0; --i) {
			if (((x >> i) & 1) == 1 && !got) got = true;
			if (((x >> i) & 1) == 1 || got)
				x_bits.push_back((x >> i) & 1);
		}
		got = false;
		x = x_bits.size();
		for (int i = 63; i >= 0; --i) {
			if (((x >> i) & 1) == 1 && !got) got = true;
			if (((x >> i) & 1) == 1 || got)
				len_bits.push_back((x >> i) & 1);
		}

		if (x == 0) {
			x_bits.push_back(0);
			len_bits.push_back(1);
		}

		BitVector res = to_gamma_code(len_bits);
		for (int i = 0; i < x_bits.size(); ++i) {
			res.push_back(x_bits[i]);
		}
		return res;
	}

	void write_bits(BitVector bits) {
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