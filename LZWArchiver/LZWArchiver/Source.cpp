#include <iostream>
#include <fstream>
#include "Trie.h"
#include "FileWriter.h"
#include "FileReader.h"
#include <windows.h>
#include <algorithm>
#include <string>

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

	BitVector bits = writer.get_bits(is_dynamic, 1);
	writer.write_bits(bits);

	bits = writer.get_bits(tr.get_size(), 16);
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

std::pair<std::vector<uint8_t>, std::string> read_unzip(FileReader& reader) {
	bool is_dynamic = reader.get_from_n_bits(1);
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

	std::string extension = "";
	int zeros = 0;
	std::vector<uint8_t> file_bytes;

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
			for (int j = 0; j < vec_curr.size(); ++j) {
				if (zeros < 2) {
					if (vec_curr[j] == 0)
						++zeros;
					else
						extension.push_back((char)vec_curr[j]);
				}
				else
					file_bytes.push_back(vec_curr[j]);
			}
			add = vec_curr.size();
			curr = bytes.size() - 1;
			need_to_add = false;
		}
		else {
			for (int j = 0; j < bytes[code].size(); ++j) {
				if (zeros < 2) {
					if (bytes[code][j] == 0)
						++zeros;
					else
						extension.push_back((char)bytes[code][j]);
				} else
					file_bytes.push_back(bytes[code][j]);
			}
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
	return std::make_pair(file_bytes, extension);
}

bool check_file(std::string path) {
	std::fstream stream(path, std::fstream::in);
	bool is_ok = stream.is_open() && stream.good();
	if (is_ok)
		stream.close();
	return is_ok;
}

void run(std::string operation, std::string file_path, bool mode) {
	if (!check_file(file_path)) {
		std::cout << "Invalid file path\n";
		return;
	}

	if (operation == "zip") {
		std::vector<uint8_t> file_buff = get_file_bytes(file_path);

		std::reverse(file_buff.begin(), file_buff.end());
		file_buff.push_back(0);
		uint64_t last_backslash = file_path.rfind('\\');
		uint64_t ext_index = file_path.rfind('.');
		if (ext_index > last_backslash) {
			std::string ext = file_path.substr(ext_index + 1);
			for (int i = ext.size() - 1; i >= 0; --i) {
				file_buff.push_back((uint8_t)ext[i]);
			}
		}
		file_buff.push_back(0);
		std::reverse(file_buff.begin(), file_buff.end());

		FileWriter writer(get_file_save_path("kalov"));
		write_zip(file_buff, writer, mode);
		std::cout << "Done\n";
	} else if (operation == "unzip") {
		FileReader reader(file_path);
		std::pair<std::vector<uint8_t>, std::string> file_unzipped = read_unzip(reader);
		std::ofstream out(get_file_save_path(file_unzipped.second), std::ios::binary);
		out.write((char*)&file_unzipped.first[0], file_unzipped.first.size());
		out.close();
		std::cout << "Done\n";
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
		run(argv[1], argv[2], false);
	}
	else if (argc == 4) {
		run(argv[1], argv[2], argv[3] == "1");
	}
	else if (argc > 1) {
		std::cout << "Invalid args provided\n";
		return 0;
	}

	while (true) {
		std::string operation, path, mode = "0";
		std::cin >> operation;
		if (operation == "?") {
			std::cout << "zip {0/1 is_dynamic_mode} {path} - file archiving using the LZW algorithm\nunzip {path} - file dearchiving\n";
			continue;
		}
		if (operation == "zip") {
			std::cin >> mode;
		}
		std::cin.ignore();
		
		std::getline(std::cin, path);

		if (path[0] == '\"') path.erase(path.begin());
		if (*path.rbegin() == '\"') path.pop_back();

		run(operation, path, mode == "1");
	}
}
