#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <windows.h>
#include <atlbase.h>
#include <atlconv.h>
#include <format>
#include <set>
#include <algorithm>
#include <stdlib.h>
#include <stdarg.h>

class BitVector {
private:
	size_t _value;
	size_t _sz;
public:
	BitVector(size_t sz = 0) {
		_value = 0;
		_sz = sz;
	}
	size_t size() {
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
	void set(size_t ind, bool val) {
		if (this->operator[](ind))
			_value ^= 1LL << (63 - ind);
		_value |= (1LL << (63 - ind)) * val;
	}
	void set_value(size_t new_val) {
		_value = new_val;
	}
	bool empty() {
		return _sz == 0;
	}
	size_t value() {
		return _value;
	}
	void reverse() {
		int i = 0, j = _sz - 1;
		while (i < j) {
			bool tmp = this->operator[](i);
			set(i, this->operator[](j));
			set(j, tmp);
		}
	}

	bool operator[](size_t ind) {
		return (_value >> (63 - ind)) & 1;
	}
	bool operator==(BitVector& a) {
		return _value == a.value();
	}
	bool operator<(BitVector& a) {
		return _value < a.value();
	}
};

std::vector<uint8_t> get_file_bytes(std::string path) {
	std::ifstream file_stream(path, std::fstream::binary);
	if (!file_stream || !file_stream.good()) {
		throw std::exception("Invalid file path");
	}

	std::vector<uint8_t> buff((std::istreambuf_iterator<char>(file_stream)), (std::istreambuf_iterator<char>()));
	file_stream.close();
	return buff;
}

std::string get_file_save_path(std::string extension) {
	std::string filter = "Resulting file (*." + extension + ")0*." + extension + "0All files (*.*)0*.*0";

	std::replace(filter.begin(), filter.end(), '0', '\0');

	OPENFILENAME ofn;

	char szFileName[MAX_PATH] = "";

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = filter.c_str();
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = extension.c_str();

	GetSaveFileName(&ofn);
	return ofn.lpstrFile;
}

struct Node {
	Node* L, *R;
	uint8_t val;
	Node() {
		L = R = nullptr;
		val = 0;
	}
	Node(uint8_t val) {
		L = R = nullptr;
		this->val = val;
	}
	bool is_leaf() {
		return L == R && R == nullptr;
	}
};

Node* generate_tree(std::vector<uint8_t>& file_buff) {
	std::vector<int> freq(256, 0);
	for (size_t i = 0; i < file_buff.size(); ++i) {
		++freq[file_buff[i]];
	}

	std::set<std::pair<std::pair<int, int>, Node*>> prior;
	for (int i = 0; i < 256; ++i) {
		if (freq[i] != 0)
			prior.insert(std::make_pair(std::make_pair(freq[i], -i), new Node(i)));
	}

	int _time = 256;
	while (prior.size() > 1) {
		std::pair<std::pair<int, int>, Node*> top1 = *prior.begin();
		prior.erase(prior.begin());
		std::pair<std::pair<int, int>, Node*> top2 = *prior.begin();
		prior.erase(prior.begin());

		Node* new_node = new Node();
		new_node->L = top1.second;
		new_node->R = top2.second;
		prior.insert(std::make_pair(std::make_pair(top1.first.first + top2.first.first, -_time), new_node));
		++_time;
	}
	return prior.begin()->second;
}

std::vector<BitVector> generate_table(Node* p) {
	std::vector<BitVector> table(256);
	BitVector curr;
	auto dfs = [&](Node* p, auto f) {
		if (p->is_leaf()) {
			table[p->val] = curr;
			return;
		}
		if (p->L != nullptr) {
			curr.push_back(0);
			f(p->L,f);
			curr.pop_back();
		}
		if (p->R != nullptr) {
			curr.push_back(1);
			f(p->R, f);
			curr.pop_back();
		}
	};

	dfs(p, dfs);
	return table;
}

std::vector<std::pair<uint8_t, BitVector>> generate_table(std::vector<uint8_t>& file_buff) {
	Node* p = generate_tree(file_buff);
	std::vector<BitVector> table = generate_table(p);
	std::vector<std::pair<uint8_t, BitVector>> filtered;
	for (int i = 0; i < 256; ++i) {
		if (table[i].empty()) continue;
		filtered.push_back(std::make_pair(i, table[i]));
	}
	return filtered;
}

BitVector to_gamma_code(BitVector& bits) {
	BitVector added = bits;
	added.push_front(1);
	int sz = (int)added.size() - 1;
	for (int i = 0; i < sz; ++i) {
		added.push_front(0);
	}
	return added;
}

void write(std::ofstream& file, BitVector& bits, uint8_t& curr_byte, int& curr_pos) {
	int written = 0;
	while (bits.size() != written) {
		while (curr_pos < 8 && bits.size() != written) {
			if (bits[written] == 1)
				curr_byte |= 1LL << (7 - curr_pos);
			++curr_pos;
			++written;
		}
		if (curr_pos == 8) {
			file.write((char*)&curr_byte, sizeof(uint8_t));
			curr_pos = 0;
			curr_byte = 0;
		} else break;
	}
}

BitVector get_bits(size_t n, int bits_count) {
	BitVector vec;
	for (int i = 0; i < bits_count; ++i) {
		vec.push_back((n >> (bits_count - 1 - i)) & 1);
	}
	return vec;
}

void write_table(std::vector<std::pair<uint8_t, BitVector>>& huffman_table, uint8_t& curr_byte, int& curr_pos, std::ofstream& stream) {
	BitVector bits = get_bits(huffman_table.size(), 32);
	write(stream, bits, curr_byte, curr_pos);
	for (int i = 0; i < huffman_table.size(); ++i) {
		bits = get_bits(huffman_table[i].first, 8);
		write(stream, bits, curr_byte, curr_pos);
		bits = to_gamma_code(huffman_table[i].second);
		write(stream, bits, curr_byte, curr_pos);
	}
}

bool get_next_bit(uint8_t& curr_byte, int& curr_pos, std::ifstream& stream) {
	bool val = (curr_byte >> (7 - curr_pos)) & 1;
	++curr_pos;
	if (curr_pos == 8) {
		curr_pos = 0;
		stream.read((char*)&curr_byte, sizeof(curr_byte));
	}
	return val;
}

size_t get_from_n_bits(int n, uint8_t& curr_byte, int& curr_pos, std::ifstream& stream) {
	size_t val = 0;
	for (int i = 0; i < n; ++i) {
		val |= get_next_bit(curr_byte, curr_pos, stream) * (1LL << (n - i - 1));
	}
	return val;
}

void write_zip(std::vector<uint8_t>& file_buff, std::vector<std::pair<uint8_t, BitVector>>& huffman_table, uint8_t& curr_byte, int& curr_pos) {
	std::ofstream stream(get_file_save_path("bruh"), std::fstream::trunc | std::fstream::binary);

	write_table(huffman_table, curr_byte, curr_pos, stream);
	BitVector bits = get_bits(file_buff.size(), 32);

	std::vector<std::pair<uint8_t, BitVector*>> nums;
	for (int i = 0; i < huffman_table.size(); ++i) {
		nums.push_back(std::make_pair(huffman_table[i].first, &huffman_table[i].second));
	}
	std::sort(nums.begin(), nums.end());
	
	write(stream, bits, curr_byte, curr_pos);
	for (int i = 0; i < file_buff.size(); ++i) {
		auto it = std::lower_bound(nums.begin(), nums.end(), std::make_pair(file_buff[i], (BitVector*)nullptr));
		write(stream, *it->second, curr_byte, curr_pos);
	}
	if (curr_pos != 0) {
		stream.write((char*)&curr_byte, sizeof(uint8_t));
	}
	stream.close();
}

BitVector from_gamma_code(uint8_t& curr_byte, int& curr_pos, std::ifstream& stream) {
	BitVector bits;
	int n = 0;
	while (get_next_bit(curr_byte, curr_pos, stream) == 0) {
		++n;
	}
	for (int j = 0; j < n; ++j) {
		bits.push_back(get_next_bit(curr_byte, curr_pos, stream));
	}
	return bits;
}

std::vector<std::pair<uint8_t, BitVector>> read_table(uint8_t& curr_byte, int& curr_pos, std::ifstream& stream) {
	int table_size = get_from_n_bits(32, curr_byte, curr_pos, stream);
	std::vector<std::pair<uint8_t, BitVector>> table;
	for (int i = 0; i < table_size; ++i) {
		uint8_t c = get_from_n_bits(8, curr_byte, curr_pos, stream);
		table.push_back(std::make_pair(c, from_gamma_code(curr_byte, curr_pos, stream)));
	}
	return table;
}

std::pair<std::vector<uint8_t>, std::string> read_unzip(uint8_t& curr_byte, int& curr_pos, std::ifstream& stream) {
	std::vector<std::pair<uint8_t, BitVector>> table = read_table(curr_byte, curr_pos, stream);
	
	std::vector<std::pair<std::pair<size_t, int>, uint8_t>> nums;
	for (int i = 0; i < table.size(); ++i) {
		nums.push_back(std::make_pair(std::make_pair(table[i].second.value(), table[i].second.size()), table[i].first));
	}
	std::sort(nums.begin(), nums.end());

	std::vector<uint8_t> buffer;
	int sz = get_from_n_bits(32, curr_byte, curr_pos, stream);

	std::string extension = "";
	int zeros = 0;
	size_t curr_bits = 0;
	int curr_sz = 0;
	while (buffer.size() != sz) {
		bool bit = get_next_bit(curr_byte, curr_pos, stream);
		curr_bits |= bit * (1LL << (63 - curr_sz));
		++curr_sz;
		auto it = std::lower_bound(nums.begin(), nums.end(), std::make_pair(std::make_pair(curr_bits, curr_sz), (uint8_t)0));
		if (it == nums.end() || it->first.first != curr_bits || it->first.second != curr_sz)
			continue;
		curr_bits = 0;
		curr_sz = 0;
		if (zeros >= 2)
			buffer.push_back(it->second);
		else {
			if (it->second != 0)
				extension.push_back(it->second);
			else
				++zeros;
		}
	}
	return std::make_pair(buffer, extension);
}

bool check_file(std::string path) {
	std::fstream stream(path, std::fstream::in);
	bool is_ok = stream.is_open() && stream.good();
	stream.close();
	return is_ok;
}

void run(std::string operation, std::string file_path) {
	if (!check_file(file_path)) {
		std::cout << "Invalid file path\n";
		return;
	}

	if (operation == "zip") {
		std::vector<uint8_t> file_buff = get_file_bytes(file_path);

		std::reverse(file_buff.begin(), file_buff.end());
		file_buff.push_back(0);
		size_t last_backslash = file_path.rfind('\\');
		size_t ext_index = file_path.rfind('.');
		if (ext_index > last_backslash) {
			std::string ext = file_path.substr(ext_index + 1);
			for (int i = ext.size() - 1; i >= 0; --i) {
				file_buff.push_back((uint8_t)ext[i]);
			}
		}
		file_buff.push_back(0);
		std::reverse(file_buff.begin(), file_buff.end());
		
		std::vector<std::pair<uint8_t, BitVector>> huffman_table = generate_table(file_buff);

		uint8_t curr_byte = 0;
		int curr_pos = 0;
		write_zip(file_buff, huffman_table, curr_byte, curr_pos);
		std::cout << "Done\n";
	} else if (operation == "unzip") {
		uint8_t curr_byte = 0;
		int curr_pos = 0;
		std::ifstream stream(file_path, std::fstream::binary);
		stream.read((char*)&curr_byte, sizeof(curr_byte));
		std::pair<std::vector<uint8_t>, std::string> file_unzipped = read_unzip(curr_byte, curr_pos, stream);
		std::ofstream out(get_file_save_path(file_unzipped.second), std::ios::binary);
		out.write((char*)&file_unzipped.first[0], file_unzipped.first.size());
		out.close();
		std::cout << "Done\n";
	} else if (operation == "gen") {
		std::vector<uint8_t> file_buff = get_file_bytes(file_path);
		std::vector<std::pair<uint8_t, BitVector>> huffman_table = generate_table(file_buff);

		std::cout << "Table for encrypting bytes in a file:\n";
		for (int i = 0; i < huffman_table.size(); ++i) {
			std::cout << (int)huffman_table[i].first << " <-> ";
			for (int j = 0; j < huffman_table[i].second.size(); ++j) {
				std::cout << huffman_table[i].second[j];
			}
			std::cout << '\n';
		}
		std::cout << "Do you want to save it to a file? (y/n)\n";
		char c;
		std::cin >> c;
		if (c == 'y' || (c ^ 'y') == 32) {
			std::ofstream stream(get_file_save_path("bruhtb"), std::fstream::trunc | std::fstream::binary);
			uint8_t curr_byte = 0;
			int curr_pos = 0;
			write_table(huffman_table, curr_byte, curr_pos, stream);
			if (curr_pos != 0) {
				stream.write((char*)&curr_byte, sizeof(uint8_t));
			}
			stream.close();
			std::cout << "Done\n";
		} else {
			return;
		}
	} else if (operation == "regen") {
		uint8_t curr_byte = 0;
		int curr_pos = 0;
		std::ifstream stream(file_path, std::fstream::binary);
		stream.read((char*)&curr_byte, sizeof(curr_byte));
		std::vector<std::pair<uint8_t, BitVector>> huffman_table = read_table(curr_byte, curr_pos, stream);
		std::cout << "Unzipped table for encrypting bytes in a file:\n";
		for (int i = 0; i < huffman_table.size(); ++i) {
			std::cout << (int)huffman_table[i].first << " <-> ";
			for (int j = 0; j < huffman_table[i].second.size(); ++j) {
				std::cout << huffman_table[i].second[j];
			}
			std::cout << '\n';
		}
		stream.close();
	} else {
		std::cout << "Invalid operation type\n";
		return;
	}
}

int main(int argc, char* argv[]) {
	if (argc == 3) {
		run(argv[1], argv[2]);
	}
	if (argc != 3 && argc > 1) {
		std::cout << "Invalid args provided\n";
		return 0;
	}

	while (true) {
		std::string operation, path;
		std::cin >> operation;
		if (operation == "?") {
			std::cout << "zip - file archiving using the Huffman algorithm\nunzip - file dearchiving\ngen - table generation for coding\nregen - retrieving a table from a file\n";
			continue;
		}
		std::cin >> path;
		run(operation, path);
	}
}