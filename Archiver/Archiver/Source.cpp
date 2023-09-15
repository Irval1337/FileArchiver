#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <windows.h>
#include <atlbase.h>
#include <atlconv.h>
#include <algorithm>
#include <stdlib.h>
#include <stdarg.h>
#include "BitVector.h"
#include "HuffmanCoder.h"
#include "FileReader.h"
#include "FileWriter.h"

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
	if (extension.size() == 0)
		filter = "All files (*.*)0*.*0";

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

void write_table(std::vector<std::pair<uint8_t, BitVector>>& huffman_table, FileWriter& writer) {
	BitVector bits = writer.get_bits(huffman_table.size(), 32);
	writer.write_bits(bits);
	for (int i = 0; i < huffman_table.size(); ++i) {
		bits = writer.get_bits(huffman_table[i].first, 8);
		writer.write_bits(bits);
		bits = writer.to_gamma_code(huffman_table[i].second);
		writer.write_bits(bits);
	}
}

void write_zip(std::vector<uint8_t>& file_buff, std::vector<std::pair<uint8_t, BitVector>>& huffman_table) {
	FileWriter writer(get_file_save_path("bruh"));
	write_table(huffman_table, writer);

	std::vector<std::pair<uint8_t, BitVector*>> nums;
	for (int i = 0; i < huffman_table.size(); ++i) {
		nums.push_back(std::make_pair(huffman_table[i].first, &huffman_table[i].second));
	}
	std::sort(nums.begin(), nums.end());
	
	BitVector bits = writer.get_bits(file_buff.size(), 32);
	writer.write_bits(bits);
	for (int i = 0; i < file_buff.size(); ++i) {
		auto it = std::lower_bound(nums.begin(), nums.end(), std::make_pair(file_buff[i], (BitVector*)nullptr));
		writer.write_bits(*it->second);
	}
}

std::vector<std::pair<uint8_t, BitVector>> read_table(FileReader& reader) {
	int table_size = reader.get_from_n_bits(32);
	std::vector<std::pair<uint8_t, BitVector>> table;
	for (int i = 0; i < table_size; ++i) {
		uint8_t c = reader.get_from_n_bits(8);
		table.push_back(std::make_pair(c, reader.from_gamma_code()));
	}
	return table;
}

std::pair<std::vector<uint8_t>, std::string> read_unzip(std::string path) {
	FileReader reader(path);
	std::vector<std::pair<uint8_t, BitVector>> table = read_table(reader);
	
	std::vector<std::pair<std::pair<size_t, int>, uint8_t>> nums;
	for (int i = 0; i < table.size(); ++i) {
		nums.push_back(std::make_pair(std::make_pair(table[i].second.value(), table[i].second.size()), table[i].first));
	}
	std::sort(nums.begin(), nums.end());

	std::vector<uint8_t> buffer;
	int sz = reader.get_from_n_bits(32);

	std::string extension = "";
	int zeros = 0;
	size_t curr_bits = 0;
	int curr_sz = 0;
	while (buffer.size() + extension.size() + 2 != sz) {
		bool bit = reader.get_next_bit();
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
	if (is_ok)
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

		HuffmanCoder coder;
		coder.generate_table(file_buff);
		
		std::vector<std::pair<uint8_t, BitVector>> huffman_table = coder.get_table();
		write_zip(file_buff, huffman_table);
		std::cout << "Done\n";
	} else if (operation == "unzip") {
		std::pair<std::vector<uint8_t>, std::string> file_unzipped = read_unzip(file_path);
		std::ofstream out(get_file_save_path(file_unzipped.second), std::ios::binary);
		out.write((char*)&file_unzipped.first[0], file_unzipped.first.size());
		out.close();
		std::cout << "Done\n";
	} else if (operation == "gen") {
		std::vector<uint8_t> file_buff = get_file_bytes(file_path);

		HuffmanCoder coder;
		coder.generate_table(file_buff);
		std::vector<std::pair<uint8_t, BitVector>> huffman_table = coder.get_table();

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
			FileWriter writer(get_file_save_path("bruhtb"));
			write_table(huffman_table, writer);
			std::cout << "Done\n";
		} else {
			return;
		}
	} else if (operation == "regen") {
		FileReader reader(file_path);
		std::vector<std::pair<uint8_t, BitVector>> huffman_table = read_table(reader);
		std::cout << "Unzipped table for encrypting bytes in a file:\n";
		for (int i = 0; i < huffman_table.size(); ++i) {
			std::cout << (int)huffman_table[i].first << " <-> ";
			for (int j = 0; j < huffman_table[i].second.size(); ++j) {
				std::cout << huffman_table[i].second[j];
			}
			std::cout << '\n';
		}
	} else {
		std::cout << "Invalid operation type\n";
		return;
	}
}

int main(int argc, char* argv[]) {
	setlocale(LC_ALL, "Russian");

	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	
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
		std::cin.ignore();
		std::getline(std::cin, path);

		if (path[0] == '\"') path.erase(path.begin());
		if (*path.rbegin() == '\"') path.pop_back();

		run(operation, path);
	}
}
