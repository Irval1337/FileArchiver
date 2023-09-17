#include <iostream>
#include <fstream>
#include "Trie.h"
#include "FileWriter.h"
#include "FileReader.h"

std::vector<uint8_t> get_file_bytes(std::string path) {
	std::ifstream file_stream(path, std::fstream::binary);
	if (!file_stream || !file_stream.good()) {
		throw std::exception("Invalid file path");
	}
	std::vector<uint8_t> buff((std::istreambuf_iterator<char>(file_stream)), (std::istreambuf_iterator<char>()));
	file_stream.close();
	return buff;
}

inline int get_bits(int size) {
	int lg = log2(size + 1);
	if ((1LL << lg) < size + 1)
		++lg;
	return lg;
}

void write_zip(std::vector<uint8_t>& buffer, FileWriter& writer, bool is_dynamic) {
	Trie tr;
	int cnt[256] = { 0 };
	for (int i = 0; i < buffer.size(); ++i) {
		cnt[buffer[i]]++;
	}

	std::vector<uint8_t> vec(1, 0);
	for (int i = 0; i < 256; ++i) {
		if (cnt[i] == 0) continue;
		vec[0] = i;
		tr.add(vec.begin(), vec.end(), tr.get_size(), nullptr);
	}

	BitVector bits = writer.get_bits(tr.get_size(), 16);
	writer.write_bits(bits);
	for (int i = 0; i < 256; ++i) {
		if (cnt[i] == 0) continue;
		bits = writer.get_bits(i, 8);
		writer.write_bits(bits);
	}

	bits = writer.get_bits(buffer.size(), 32);
	writer.write_bits(bits);

	TrieNode* prev = nullptr;
	vec.clear();
	int bits_count = get_bits(tr.get_size());
	for (int i = 0; i < buffer.size();) {
		uint8_t curr_byte = buffer[i];
		vec.push_back(curr_byte);
		auto ptr = tr.find(vec.end() - 1, vec.end(), prev);
		if (ptr == nullptr) {
			vec.pop_back();
			if (!is_dynamic)
				bits = writer.get_bits(*prev->get_code(), bits_count);
			else
				bits = writer.to_delta_code(*prev->get_code());
			writer.write_bits(bits);
			vec.push_back(curr_byte);
			tr.add(vec.begin(), vec.end(), tr.get_size(), nullptr);
			vec.clear();
			prev = nullptr;
		} else {
			prev = ptr;
			++i;
		}

		if (!is_dynamic) {
			bits_count = get_bits(tr.get_size());
			if (bits_count > 16) {
				vec.resize(1);
				tr.clear();
				for (int i = 0; i < 256; ++i) {
					if (cnt[i] == 0) continue;
					vec[0] = i;
					tr.add(vec.begin(), vec.end(), tr.get_size(), nullptr);
				}
				bits_count = get_bits(tr.get_size());
				vec.clear();
				prev = nullptr;
			}
		}
	}

	if (!vec.empty()) {
		bits = writer.get_bits(*prev->get_code(), bits_count);
		writer.write_bits(bits);
	}
}

void read_unzip(FileReader& reader, FileWriter& writer, bool is_dynamic) {
	int sz = reader.get_from_n_bits(16);
	std::vector<uint8_t> vec(1);
	std::vector<std::vector<uint8_t>> bytes;
	std::vector<std::vector<uint8_t>> alph;
	for (int i = 0; i < sz; ++i) {
		uint8_t byte = reader.get_from_n_bits(8);
		vec[0] = byte;
		bytes.push_back(vec);
		alph.push_back(vec);
	}

	int bits_count = get_bits(bytes.size());
	int len = reader.get_from_n_bits(32);
	vec.clear();
	int prev = -1;
	bool first = true;
	for (int i = 0; i < len;) {
		if (!is_dynamic && bits_count > 16) {
			bytes = alph;
			bits_count = get_bits(bytes.size());
			first = true;
		}

		int code;
		if (!is_dynamic)
			code = reader.get_from_n_bits(bits_count);
		else
			code = reader.from_delta_code();
		int curr, add;
		bool need_to_add = true;
		if (code >= bytes.size()) {
			std::vector<uint8_t> vec_curr = bytes[prev];
			vec_curr.push_back(bytes[prev][0]);
			bytes.push_back(vec_curr);
			for (int j = 0; j < vec_curr.size(); ++j)
				writer.write_bits(writer.get_bits(vec_curr[j], 8));
			add = vec_curr.size();
			curr = bytes.size() - 1;
			need_to_add = false;
		}
		else {
			for (int j = 0; j < bytes[code].size(); ++j)
				writer.write_bits(writer.get_bits(bytes[code][j], 8));
			add = bytes[code].size();
			curr = code;
		}
		
		if (first) {
			i += add;
			bits_count = get_bits(bytes.size() + 1);
			prev = curr;
			first = false;
			continue;
		}
		std::vector<uint8_t> vec_curr = bytes[prev];
		vec_curr.push_back(bytes[curr][0]);
		if (need_to_add)
			bytes.push_back(vec_curr);
		prev = curr;
		bits_count = get_bits(bytes.size() + 1);
		i += add;
	}
}

int main() {
	FileWriter w("C:\\1\\3.jpg");
	FileReader r("C:\\1\\2.txt");
	read_unzip(r, w, true);
	/*auto bytes = get_file_bytes("C:\\1\\555.jpg");
	FileWriter w("C:\\1\\2.txt");
	write_zip(bytes, w, true);*/
}
