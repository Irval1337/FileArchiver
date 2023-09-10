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

// ��������� BitVector - ���������� ���������� ��� ����� ������, ����� ������ �������� � ��� ������
class BitVector { // ����������, ������������� ��� ��� ������� vector<bool>
private:
	// � ��������� ����� ������ ���������� �������������� ����� � ����� (������� �� �������� �������) � ���� �����
	size_t _value; // ������ ����� � ���� unsigned long long (64 ���� ��������)
	size_t _sz;
public:
	// �����������
	BitVector() {
		_value = 0;
		_sz = 0;
	}
	// ���������� ������� ���������� �������������� �����
	size_t size() {
		return _sz;
	}
	// ������� ��� ���� ��� � �������������� ������ �� ���
	void push_back(bool val) {
		// �� �������� ����� ��� val �� (63 - ������� ������) �����, ������ ��������� ��� � ���������� ���������
		_value |= (1LL << (63 - _sz)) * val;
		// �������� ������
		++_sz;
	}
	// "������" ������� ���
	void pop_back() {
		// ���� ������� ��� ��� 1, �� � ������� xor ������� ��� 0
		if (this->operator[](_sz - 1))
			_value ^= 1LL << (64 - _sz); // ����� �� ���������� ���� � ��� ���� � 1
		// �������� ������
		--_sz;
	}
	// ������� ��� ���� ��� � ������ "������"
	void push_front(bool val) {
		// ����� �������� ��� ������� ����� ������ �� 1 (���������� ������� �� 2) ���� ����� ���, ��������� �� ������ �������
		_value = (_value >> 1) | ((1LL << 63) * val);
		// �������� ������
		++_sz;
	}
	// �������� �������� ���� ��� ������� ind (0-���������)
	void set(size_t ind, bool val) {
		if (this->operator[](ind)) // ���� � ���� ���� 1, �� ������� ���
			_value ^= 1LL << (63 - ind);
		_value |= (1LL << (63 - ind)) * val; // �������� ����� �������� ����� ���� � ������� ������� ��� � ������� �� ������ �������
	}
	// �������� �� "�������"
	bool empty() {
		return _sz == 0;
	}
	// ���������� ����������, � ������� �������� ��� ����
	size_t value() {
		return _value;
	}

	// �������� �������� ind-��� ����
	bool operator[](size_t ind) {
		return (_value >> (63 - ind)) & 1;
	}
};

// ������ ������� ���������� ������ uint8_t (��� ����) �� �����.
std::vector<uint8_t> get_file_bytes(std::string path) {
	// ��������� �����. ���� std::fstream::binary ����� ��� ����������� ����������
	std::ifstream file_stream(path, std::fstream::binary);
	// ���������, ��� ������ ������� ����
	if (!file_stream || !file_stream.good()) {
		throw std::exception("Invalid file path");
	}

	// � ������� ����� ���������� ���������� ���������� ��� ����� �� ����� � ������
	std::vector<uint8_t> buff((std::istreambuf_iterator<char>(file_stream)), (std::istreambuf_iterator<char>()));
	// ��������� �����
	file_stream.close();
	return buff;
}

// � ������� WinAPI ��������� ������������ ������� ���� ��������� ���� � ��� ����� ���������
std::string get_file_save_path(std::string extension) { // �������� � ���������� ����������� ���������� ��� �����
	std::string filter = "Resulting file (*." + extension + ")0*." + extension + "0All files (*.*)0*.*0"; // ����� ��� �������
	if (extension.size() == 0) // ���� ���������� ��� ����� ����������, �� � ������� ������� ��� �����
		filter = "All files (*.*)0*.*0";

	std::replace(filter.begin(), filter.end(), '0', '\0'); // �������� 0 �� \0, ����� �������� ����� ������� ���� �� �����

	OPENFILENAME ofn; // ��������� �� WinAPI ��� ������ SaveFileDialog

	char szFileName[MAX_PATH] = ""; // ������ ��� ���� � �����

	ZeroMemory(&ofn, sizeof(ofn)); // ������� ��� �������� � ��������� ofn

	ofn.lStructSize = sizeof(ofn); // ������ ���������
	ofn.hwndOwner = NULL; // �� ����� ���������� ���������� �������� ����
	ofn.lpstrFilter = filter.c_str(); // ������� ������ ��� ���������� ������
	ofn.lpstrFile = szFileName; // ������ ��� ����
	ofn.nMaxFile = MAX_PATH; // ������������ ������ ����
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY; // ����� - ������� � ����������, ������������ ���� ������ ������������, ������ ����� ������ ��� ������
	ofn.lpstrDefExt = extension.c_str(); // ���������� ��� ���������� �� ���������

	GetSaveFileName(&ofn); // �������� ������� �� WinAPI
	return ofn.lpstrFile; // ���������� ���� �� �����
}

// ��������� ��� ������� ������. ����� ��� �������� ������ ��������
struct Node {
	Node* L, *R; // ��������� �� �����
	uint8_t val; // ���� � ���� �������
	Node() { // ����������� �� ���������
		L = R = nullptr;
		val = 0;
	}
	Node(uint8_t val) { // ����������� ��� ����� - ���������� ����� �����-�� ����
		L = R = nullptr;
		this->val = val;
	}
	bool is_leaf() { // �������� �� ����
		return L == R && R == nullptr;
	}
};

// ������� ��� ��������� ������ �� ������� ������
Node* generate_tree(std::vector<uint8_t>& file_buff) {
	// ������, � ������� �� ����� ������� ������� ��������� ������� ����� 
	std::vector<int> freq(256, 0); // ����� ���������� 2^8=256 ��������� ������
	for (size_t i = 0; i < file_buff.size(); ++i) {
		++freq[file_buff[i]]; // ���������� �� ������ ����� � �������� �������
	}

	// Set ��� ����, ����� �������� ������� � ����������� �������� ���������
	// ������ ������� ����������� �� ���� pair<pair<int, int>, Node*>. ���������� �� ������ ������ ��������� ��������:
	// ������ - ������� ��������� ������� �������
	// ������ - "����� ����������". ��� ������ �� �������� ������� � set, ��� ������ ����� ��� ��������. � ������� ���
	// ��� ������ � undefined behavior ��-�� ����, ��� ��������� ������ ��� ����� ����� �����
	// ������ - ��������� �� �������
	std::set<std::pair<std::pair<int, int>, Node*>> prior;
	for (int i = 0; i < 256; ++i) {
		// ���� ������ ���� ����������� ���� �� ���� ���, �� ������� ��� � set.
		if (freq[i] != 0)
			prior.insert(std::make_pair(std::make_pair(freq[i], -i), new Node(i))); // ������� � set ((�������, -����� ����������), ��������� �� �������)
	}

	int _time = 256; // ������� ���������� � ��������. ���������� ��� 256, ��� ��� ��� ����� ������, ��� ������������ ����� �� ��� ����������� ������
	while (prior.size() > 1) { // ���� set ������� �� >= 2 ���������
		// ������� ������ ��� �������� � ������� ��
		std::pair<std::pair<int, int>, Node*> top1 = *prior.begin();
		prior.erase(prior.begin());
		std::pair<std::pair<int, int>, Node*> top2 = *prior.begin();
		prior.erase(prior.begin());

		// ������� ����� �������, ������� ���������� � ���� ��� ������ ��� ������
		Node* new_node = new Node();
		new_node->L = top1.second;
		new_node->R = top2.second;
		// �������� ��� ������� � ������������ �� ���������� (������� � �����) � set
		prior.insert(std::make_pair(std::make_pair(top1.first.first + top2.first.first, -_time), new_node));
		++_time; // �������� �����
	}
	return prior.begin()->second; // ���������� ��������� �� ������������ ���������� �������. ���������� ��� ������ ������ ��������
}

// ������� ��� ��������� ������������� ������� ��������. ������� ������������ �� ���� ����� ����� ������� 256 (��� ������� �����)
std::vector<BitVector> generate_table(Node* p) { // � ���������� ��������� ��������� �� ������ ������
	std::vector<BitVector> table(256); // �������� ���� ������� ��� ������� ���������� �����
	BitVector curr; // ������� ���
	// �������� ������� ��� dfs-������ ������
	auto dfs = [&](Node* p, auto f) { // ��� ���������� ������-�������. ��� ������ ������� ������ ������ �������. ��� �������� ����� 
		// ������������ �� ��� void dfs(Node* p). auto f � [&] ����� ��� ����������� �� � generate_table.
		if (p->is_leaf()) { // ���� ������� �������-����, �� ������� � ������� ���������� ���, ����������� � curr
			table[p->val] = curr; // val - ����, ��������������� ���� �������
			return;
		}
		if (p->L != nullptr) { // ������ � ������ ����, ���� �� ����
			curr.push_back(0); // ������� 0 � ������� ���, ��� ��� ����� �����=���������� 0
			f(p->L,f); // ��������
			curr.pop_back(); // ������ 0 �� ����
		}
		if (p->R != nullptr) { // ���������� ��� ������� ����
			curr.push_back(1);
			f(p->R, f);
			curr.pop_back();
		}
	};

	dfs(p, dfs); // �������� ����� ������ � ���������� ���������� �������
	return table;
}

// �������� ������� ��� ��������� ������� �� ������� ������ �����
std::vector<std::pair<uint8_t, BitVector>> generate_table(std::vector<uint8_t>& file_buff) {
	Node* p = generate_tree(file_buff); // ������� ������� ��������� �� ������ ������ ��������
	std::vector<BitVector> table = generate_table(p); // ������ �� ���� ���������� �������. �������, ��� � ���� ������� ����� ���� �������������� �����,
	// �� ������������� ������ ���. ����������� �� � ������� ���������� ������� � ������ filtered. ������ ������� ����� ������������ �� ���� �����
	// ��� (����, ���).
	std::vector<std::pair<uint8_t, BitVector>> filtered;
	for (int i = 0; i < 256; ++i) { // ��������� �� ���� ������, � ���� ��� ��� i-��� ����� �� ������, �� ������� ��� ���� � �������
		if (table[i].empty()) continue;
		filtered.push_back(std::make_pair(i, table[i]));
	}
	return filtered; // ������ �������
}

// �������������� ������ ����� � ������� �����-������� ������
BitVector to_gamma_code(BitVector& bits) {
	BitVector added = bits;
	added.push_front(1); // ������� � ������ 1
	int sz = (int)added.size() - 1; // ������� � ������ bits.size() - 1 (� ������ ����� �������) �����
	for (int i = 0; i < sz; ++i) {
		added.push_front(0);
	}
	return added; // ��� � ���� �������� ������-������� �������
}

// ������� ��� ������ ������ ����� bits � ����. � �������� ���������� ��������� ������ �� ����� ��� ������ (file),
// ����� ����� (bits), ������ �� ������� ������������ ���� (curr_byte) � ������� ������� � ���� ����� ��� ������ (curr_pos)
void write(std::ofstream& file, BitVector& bits, uint8_t& curr_byte, int& curr_pos) {
	int written = 0; // ���������� ���������� ����� �� ������ bits
	while (bits.size() != written) { // ���� ���� ���������������� ����
		while (curr_pos < 8 && bits.size() != written) { // ���� �� �� ����� �� ������� �������� ����� � �������� ��� ���� ��� ������
			if (bits[written] == 1) // ���� �������� ���� ����� 1
				curr_byte |= 1LL << (7 - curr_pos); // ������� ��� �� ������� curr_pos � ������� �����
			++curr_pos; // ���������� ������� ��� ������ �� 1
			++written; // �������� ���������� ���������� �����
		}
		if (curr_pos == 8) { // ���� �� ��������� ������ � ������� ����
			file.write((char*)&curr_byte, sizeof(uint8_t)); // ������� ���� ���� � ����
			curr_pos = 0; // ������� ������� ���� � ������� ��� ������ � ����
			curr_byte = 0;
		} else break; // ����� ������ �� �����, ������ ��� ���� �����������
	}
}

// ������� ��� ��������� ������ ����� �� �����. �������� ����� � ���� unsigned long long � ���������� �����, � ������� ����� �������� ��� �����
BitVector get_bits(size_t n, int bits_count) {
	BitVector vec; // ������� "������ �����"
	for (int i = 0; i < bits_count; ++i) { // ���������� �� ���� ��������� �����
		vec.push_back((n >> (bits_count - 1 - i)) & 1); // ��������� �� � ����� ������ ����� vec
	}
	return vec; // vec - ������������� ����� n � bits_count �����
}

// ������� ��� ������ ������� �������� � ����. �������� ������� �������� �� ������, ������ �� ������� ����, ������� ��� ������ � ����� �����
void write_table(std::vector<std::pair<uint8_t, BitVector>>& huffman_table, uint8_t& curr_byte, int& curr_pos, std::ofstream& stream) {
	// ������� ������� � ������ ����� ���������� ����� � ������� � ������� 32 �����. ����� ���������� �������������� ������
	BitVector bits = get_bits(huffman_table.size(), 32);
	write(stream, bits, curr_byte, curr_pos);
	// �������� �� ���� ������� � �������
	for (int i = 0; i < huffman_table.size(); ++i) {
		bits = get_bits(huffman_table[i].first, 8); // ������� ���������� � ������� 8 ����� ��� �������� ��������� �����
		write(stream, bits, curr_byte, curr_pos);
		bits = to_gamma_code(huffman_table[i].second); // ����� ���������� ���� ��� ����������. ��������� �� ���������� ������� ����������,
		// �� ������� � ���� �����-������� ������� ���� �����. ���� ��� ���������, ����� - ���������� ���� ������� ������ ������� �� ���� ����
		write(stream, bits, curr_byte, curr_pos);
	}
}

// ������� ��� ��������� ����� � ��� ����������
// ��������� - ����� �� �����, ������� �������� � ������� ���� � ������� � ���
void write_zip(std::vector<uint8_t>& file_buff, std::vector<std::pair<uint8_t, BitVector>>& huffman_table, uint8_t& curr_byte, int& curr_pos) {
	// ������� ����� ����� ��� ������. ���� trunc �������� ���������� ������������� �����, binary - ���������� ������
	// � �������� ���� ��� ���������� �������� �������� ������� get_file_save_path. ���� ������ ����� ���������� .bruh.
	std::ofstream stream(get_file_save_path("bruh"), std::fstream::trunc | std::fstream::binary);

	write_table(huffman_table, curr_byte, curr_pos, stream); // ������� ������� � ���� ������� ��������

	std::vector<std::pair<uint8_t, BitVector*>> nums; // ������ ��� ��� �������� ������ ����� � �������.
	// ������� � ���� ��� ������ �� ������� � ������� (����, ��������� �� ���). ������������ ���, �� ������ ������ �������� �������� ���� � �������
	// ��������� ������ �� ����� �������
	for (int i = 0; i < huffman_table.size(); ++i) {
		nums.push_back(std::make_pair(huffman_table[i].first, &huffman_table[i].second));
	}
	std::sort(nums.begin(), nums.end()); // ��������� ���
	
	BitVector bits = get_bits(file_buff.size(), 32);
	write(stream, bits, curr_byte, curr_pos); // ������� � ���� ���������� ������������� ������. ������� ��� ���������� ����� � ������� 32 �����.
	for (int i = 0; i < file_buff.size(); ++i) { // ���������� �� ������ �� �����
		auto it = std::lower_bound(nums.begin(), nums.end(), std::make_pair(file_buff[i], (BitVector*)nullptr)); // � ������� lower_bound
		// ������� ������������ ��� ���� � ��� ��� � ������� nums. (BitVector*)nullptr ��������� �� ���������� ������� ��� ���������, ������ ���
		// ����� ��������� ����� ������, ��� nullptr.
		write(stream, *it->second, curr_byte, curr_pos); // ������� � ���� ���� ��� �������� �������� �����
	}
	if (curr_pos != 0) { // ���� ��������� ���� �� ������, ������ ��� ����� �������� ��� � ����
		stream.write((char*)&curr_byte, sizeof(uint8_t)); // ������� ��������� ���� � ����-�����
	}
	stream.close(); // ������� ����� ��� ������
}

// ������� ��� ��������� ���������� ���� �� �����. �������� ������ �� ������� ����, ������� � ��� � ����� ��� ������
bool get_next_bit(uint8_t& curr_byte, int& curr_pos, std::ifstream& stream) {
	bool val = (curr_byte >> (7 - curr_pos)) & 1; // ������� �������� ���� �� ������� curr_pos � ������� ��������������� �����
	++curr_pos; // ���������� ������� �������
	if (curr_pos == 8) { // ���� �� ����� �� ������� �����
		curr_pos = 0; // ������� �������
		stream.read((char*)&curr_byte, sizeof(curr_byte)); // ������� ����� ���� � curr_byte
	}
	return val; // ������ �������� ���������� ����
}

// ��������� ����� �� n ��������� ����� � ������� �����. ��������� - ��� ������ + ���������� �����
size_t get_from_n_bits(int n, uint8_t& curr_byte, int& curr_pos, std::ifstream& stream) {
	size_t val = 0; // ���������� ��� ������. ����������� ���� ����, ��� n <= 64
	for (int i = 0; i < n; ++i) { // ��������� n �����
		val |= get_next_bit(curr_byte, curr_pos, stream) * (1LL << (n - i - 1)); // � ������� �������� ��� ��������� ����� ��� �� ��������������� ������� � �����
	}
	return val;
}

// ��������� ������ ����� �� �� �����-���� ������
// ��������� - ��� ������
BitVector from_gamma_code(uint8_t& curr_byte, int& curr_pos, std::ifstream& stream) {
	BitVector bits; // "������ �����"
	int n = 0;
	// ������� ���������� ����� �� ���������� ���������� ����
	while (get_next_bit(curr_byte, curr_pos, stream) == 0) {
		++n;
	}
	// ���������� � bits ����� n ��������� ����� �� �����
	for (int j = 0; j < n; ++j) {
		bits.push_back(get_next_bit(curr_byte, curr_pos, stream));
	}
	return bits;
}

// ������� ��� ���������� ������� �������� �� ����� (������� � ������ ����, �� ���� ��� �������������� ������)
std::vector<std::pair<uint8_t, BitVector>> read_table(uint8_t& curr_byte, int& curr_pos, std::ifstream& stream) {
	int table_size = get_from_n_bits(32, curr_byte, curr_pos, stream); // ������� ���������� ����� � �������. �� ���������� ��� ��� 32 ����
	std::vector<std::pair<uint8_t, BitVector>> table; // ������-�������
	for (int i = 0; i < table_size; ++i) {
		uint8_t c = get_from_n_bits(8, curr_byte, curr_pos, stream); // ������� ��������� 8 ����� - �������� ����
		table.push_back(std::make_pair(c, from_gamma_code(curr_byte, curr_pos, stream))); // ��������� ��� ���� � ������� �����-���� �������,
		// ��������� ��� ���� ��� ����� ������ � �������
	}
	return table; // ���������� ���������� �������
}

// ������� ��� ���������� ���������������� ����� � ��� �������������
std::pair<std::vector<uint8_t>, std::string> read_unzip(uint8_t& curr_byte, int& curr_pos, std::ifstream& stream) {
	std::vector<std::pair<uint8_t, BitVector>> table = read_table(curr_byte, curr_pos, stream); // ������� ������� ������� ��������
	
	std::vector<std::pair<std::pair<size_t, int>, uint8_t>> nums; // ������� ������ ��� ������� ����������� ������. �������� ���������� � write_zip
	for (int i = 0; i < table.size(); ++i) {
		// �������� ������� ������������ � ���� ((���� � ������������� ����, ���������� ����� � ����), ������������ �������� �����)
		nums.push_back(std::make_pair(std::make_pair(table[i].second.value(), table[i].second.size()), table[i].first));
	}
	std::sort(nums.begin(), nums.end());

	std::vector<uint8_t> buffer; // � ���� ������� ����� ��������� ��� ����� �����
	int sz = get_from_n_bits(32, curr_byte, curr_pos, stream); // ������� ���������� ������������� ������. ��� ����� 32 ����

	std::string extension = ""; // ����������, � ������� �������� ���������� ��������� �����
	int zeros = 0; // ���������� ������� �����. ��� ������ �� ������� 2 ������� ����, �� ������ ���������� ����������
	size_t curr_bits = 0; // ������� ��������� ���
	int curr_sz = 0; // ���������� ��������� �����
	while (buffer.size() + extension.size() + 2 != sz) { // ���� ��������� ���������� ���������� ������ �� ����� sz
		bool bit = get_next_bit(curr_byte, curr_pos, stream); // �������� ����� ��� � ��������� ��� � curr_bits
		curr_bits |= bit * (1LL << (63 - curr_sz));
		++curr_sz; // ����������� ���������� ��������� �����
		auto it = std::lower_bound(nums.begin(), nums.end(), std::make_pair(std::make_pair(curr_bits, curr_sz), (uint8_t)0)); // ���� ���� � ����� �����
		if (it == nums.end() || it->first.first != curr_bits || it->first.second != curr_sz) // ��������� ��� ����������
			continue; // ���� ������
		curr_bits = 0; // �����, �� ����� ������������ ��������� ���� (�� ����� it->second), ������� �������� curr_bits � curr_sz
		curr_sz = 0;
		if (zeros >= 2) // ���� �� ��� ������������� ���������� (���������� ��������� ����� >= 2), �� ������� ����� ���� � ������ buffer
			buffer.push_back(it->second);
		else { // ������, �� ��������� ����������
			if (it->second != 0) // ���� �������� �� 0, �� ������� ����� ������ � ������ extension
				extension.push_back(it->second);
			else // �����, �������� ���������� ��������� �����
				++zeros;
		}
	}
	return std::make_pair(buffer, extension); // ������ � �������� ���������� ���� (����� �����, ����������)
}

// ������� ��� �������� ����������� �������� �����
bool check_file(std::string path) {
	std::fstream stream(path, std::fstream::in); // ��������� �����
	bool is_ok = stream.is_open() && stream.good(); // ���� ����� ������ � �� "�������"), �� ���� �������
	if (is_ok) // ������� �����, ���� �� ������
		stream.close();
	return is_ok; // ������ ��������� �� ���������� ��������
}

void run(std::string operation, std::string file_path) {
	// �������� ������� ������ ����
	if (!check_file(file_path)) {
		std::cout << "Invalid file path\n";
		return;
	}

	// is-else ����������� ��� ������� ������� � ������ �� ��������� ��������
	if (operation == "zip") { // ���������
		std::vector<uint8_t> file_buff = get_file_bytes(file_path); // �������� ��� ����� �� �����

		// ���� ��� ���������� ���������� ����� � ������ ��� ������.
		std::reverse(file_buff.begin(), file_buff.end()); // ������� �������������� ������ ������
		file_buff.push_back(0); // ������� ������� ���� � �������� �����������
		size_t last_backslash = file_path.rfind('\\'); // ������ ��������� ������ ��������� \ � ������ file_path
		size_t ext_index = file_path.rfind('.'); // ������ ��������� ������ ��������� . � ������ file_path
		if (ext_index > last_backslash) { // ���� ��������� ����� ��������� ������ ���������� \, �� ���� ����� ����������
			std::string ext = file_path.substr(ext_index + 1); // ������, ������� ������ � ���� ���������� �����
			for (int i = ext.size() - 1; i >= 0; --i) { // ������� ��� �� ������� � ����� � �����
				file_buff.push_back((uint8_t)ext[i]);
			}
		}
		file_buff.push_back(0); // ������� ������ �������������� ������� ����
		std::reverse(file_buff.begin(), file_buff.end()); // ��������� ������ ������. ��������� �� ���������� ���������� � ����� ������ ������-������,
		// �� ������ �� ����� ��� 0[����� ����������]0[�������������� ����� �����]
		
		std::vector<std::pair<uint8_t, BitVector>> huffman_table = generate_table(file_buff); // ���������� ������� ��������

		uint8_t curr_byte = 0; // ������� ���������� curr_byte � curr_pos ��� ��������� ������ � ����
		int curr_pos = 0;
		write_zip(file_buff, huffman_table, curr_byte, curr_pos); // ���������� ��� ���� � ������ ������� write_zip
		std::cout << "Done\n";
	} else if (operation == "unzip") { // �����������
		uint8_t curr_byte = 0; // ������� ���������� curr_byte � curr_pos ��� ���������� ������ �� �����
		int curr_pos = 0;
		std::ifstream stream(file_path, std::fstream::binary); // ����� ��� ������
		stream.read((char*)&curr_byte, sizeof(curr_byte)); // ������� ������ ����
		std::pair<std::vector<uint8_t>, std::string> file_unzipped = read_unzip(curr_byte, curr_pos, stream); // � ������� ������� read_unzip ������������ ����
		stream.close(); // ������� ����� ��� ������
		std::ofstream out(get_file_save_path(file_unzipped.second), std::ios::binary); // ��������� ����� ��� ���������� �� ���� get_file_save_path,
		// � �������� �������� ���������� ������ file_unzipped.second
		out.write((char*)&file_unzipped.first[0], file_unzipped.first.size()); // ������� ��� ����� � �����
		out.close(); // ������� ���
		std::cout << "Done\n";
	} else if (operation == "gen") { // ��������� ������� ��������
		// �������� ����� �� �����, ��� �������� ����� ��������� �������
		std::vector<uint8_t> file_buff = get_file_bytes(file_path);

		std::vector<std::pair<uint8_t, BitVector>> huffman_table = generate_table(file_buff); // ���������� �������

		std::cout << "Table for encrypting bytes in a file:\n"; // ������� �� � �������� �������
		for (int i = 0; i < huffman_table.size(); ++i) {
			std::cout << (int)huffman_table[i].first << " <-> ";
			for (int j = 0; j < huffman_table[i].second.size(); ++j) {
				std::cout << huffman_table[i].second[j];
			}
			std::cout << '\n';
		}
		std::cout << "Do you want to save it to a file? (y/n)\n";
		char c;
		std::cin >> c; // ���������, ����� �� ������������ ��������� �� � ����
		if (c == 'y' || (c ^ 'y') == 32) { // c == 'y' || c == 'Y'
			std::ofstream stream(get_file_save_path("bruhtb"), std::fstream::trunc | std::fstream::binary); // ����� ��� ������ ������, ���������� - bruhtb
			uint8_t curr_byte = 0; // ������� ���������� curr_byte � curr_pos ��� ��������� ������ � ����
			int curr_pos = 0;
			write_table(huffman_table, curr_byte, curr_pos, stream); // ���������� ������ �������
			if (curr_pos != 0) { // ���� ��������� ���� �� ������, �� ������� � ���
				stream.write((char*)&curr_byte, sizeof(uint8_t));
			}
			stream.close(); // ������� �����
			std::cout << "Done\n";
		} else {
			return;
		}
	} else if (operation == "regen") { // ��������� ������� �������� �� �����
		uint8_t curr_byte = 0; // ������� ���������� curr_byte � curr_pos ��� ���������� ������ �� �����
		int curr_pos = 0;
		std::ifstream stream(file_path, std::fstream::binary); // ����� ��� ������
		stream.read((char*)&curr_byte, sizeof(curr_byte)); // ��������� ������ ����
		std::vector<std::pair<uint8_t, BitVector>> huffman_table = read_table(curr_byte, curr_pos, stream); // ��������� ������� ��������
		std::cout << "Unzipped table for encrypting bytes in a file:\n"; // ������� �� � �������� �������
		for (int i = 0; i < huffman_table.size(); ++i) {
			std::cout << (int)huffman_table[i].first << " <-> ";
			for (int j = 0; j < huffman_table[i].second.size(); ++j) {
				std::cout << huffman_table[i].second[j];
			}
			std::cout << '\n';
		}
		stream.close(); // ��������� �����
	} else { // ����������� ��������
		std::cout << "Invalid operation type\n";
		return;
	}
}

// ����� ����� � ���������. ��������� argc � argv ����� ��� ������� ����� �������. argc - ���������� ����������, argv - ���� ��������� � ���� �����.
// ���� ��������� ����������� �� ����� �������/��� ����������, �� argc = 1, argv - ���� �� ������������ �����
int main(int argc, char* argv[]) {
	if (argc == 3) { // ���� ��� ���� 2 ���. ���������, �� �������� ����������� ��������
		run(argv[1], argv[2]);
	}
	if (argc != 3 && argc > 1) { // ���� ���������� ���. ���������� ���������� �� 2, �� ������� ���������� �� ������
		std::cout << "Invalid args provided\n";
		return 0;
	}

	while (true) { // ����������� ����
		std::string operation, path; // ������� �������� � ���� �� �����
		std::cin >> operation;
		if (operation == "?") { // ���� �������� ��� "?", �� ������� "����������" ��� ������ � ����������
			std::cout << "zip - file archiving using the Huffman algorithm\nunzip - file dearchiving\ngen - table generation for coding\nregen - retrieving a table from a file\n";
			continue;
		}
		std::cin >> path;
		run(operation, path); // �������� �������� �������
	}
}