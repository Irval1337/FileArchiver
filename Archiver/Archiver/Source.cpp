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

// Структура BitVector - фактически надстройка над целым числом, чтобы удобно работать с его битами
class BitVector { // ПОЖАЛУЙСТА, воспринимайте его как простой vector<bool>
private:
	// В приватных полях храним количество использованных битов в числе (начиная со старшего разряда) и само число
	size_t _value; // Храним число в виде unsigned long long (64 бита максимум)
	size_t _sz;
public:
	// Конструктор
	BitVector() {
		_value = 0;
		_sz = 0;
	}
	// Возвращаем текущее количество использованных битов
	size_t size() {
		return _sz;
	}
	// Добавим еще один бит к использованным справа от них
	void push_back(bool val) {
		// Мы сдвигаем новый бит val на (63 - текущий размер) влево, делаем побитовое или с предыдущим значением
		_value |= (1LL << (63 - _sz)) * val;
		// Увеличим размер
		++_sz;
	}
	// "Удалим" крайний бит
	void pop_back() {
		// Если крайний бит это 1, то с помощью xor сделаем его 0
		if (this->operator[](_sz - 1))
			_value ^= 1LL << (64 - _sz); // Сдвиг до последнего бита и его ксор с 1
		// Уменьшим размер
		--_sz;
	}
	// Добавим еще один бит в начало "списка"
	void push_front(bool val) {
		// Новое значение это битовый сдвиг вправо на 1 (фактически деление на 2) плюс новый бит, сдвинутый на первую позицию
		_value = (_value >> 1) | ((1LL << 63) * val);
		// Увеличим размер
		++_sz;
	}
	// Изменить значение бита под номером ind (0-нумерация)
	void set(size_t ind, bool val) {
		if (this->operator[](ind)) // Если в бите была 1, то занулим его
			_value ^= 1LL << (63 - ind);
		_value |= (1LL << (63 - ind)) * val; // Присвоим новое значение этому биту с помощью битовых или и сдвигов на нужную позицию
	}
	// Проверка на "пустоту"
	bool empty() {
		return _sz == 0;
	}
	// Возвращаем переменную, в которой хранятся все биты
	size_t value() {
		return _value;
	}

	// Получаем значение ind-ого бита
	bool operator[](size_t ind) {
		return (_value >> (63 - ind)) & 1;
	}
};

// Данная функция возвращает вектор uint8_t (это байт) из файла.
std::vector<uint8_t> get_file_bytes(std::string path) {
	// Открываем поток. Флаг std::fstream::binary нужен для побайтового считывания
	std::ifstream file_stream(path, std::fstream::binary);
	// Проверяем, что смогли открыть файл
	if (!file_stream || !file_stream.good()) {
		throw std::exception("Invalid file path");
	}

	// С помощью магии встроенных итераторов запихиваем все байты из файла в вектор
	std::vector<uint8_t> buff((std::istreambuf_iterator<char>(file_stream)), (std::istreambuf_iterator<char>()));
	// Закрываем поток
	file_stream.close();
	return buff;
}

// С помощью WinAPI позволяем пользователю выбрать куда сохранить файл и под каким названием
std::string get_file_save_path(std::string extension) { // Передаем в аргументах необходимое расширение для файла
	std::string filter = "Resulting file (*." + extension + ")0*." + extension + "0All files (*.*)0*.*0"; // Маска для фильтра
	if (extension.size() == 0) // Если расширение для файла неизвестно, то в фильтре выберем все файлы
		filter = "All files (*.*)0*.*0";

	std::replace(filter.begin(), filter.end(), '0', '\0'); // Заменяем 0 на \0, чтобы отделить части фильтра друг от друга

	OPENFILENAME ofn; // Структура из WinAPI для вызова SaveFileDialog

	char szFileName[MAX_PATH] = ""; // Буффер для пути к файлу

	ZeroMemory(&ofn, sizeof(ofn)); // Занулим все значения в структуре ofn

	ofn.lStructSize = sizeof(ofn); // Размер структуры
	ofn.hwndOwner = NULL; // Не будем передавать дескриптор текущего окна
	ofn.lpstrFilter = filter.c_str(); // Запишем фильтр для расширений файлов
	ofn.lpstrFile = szFileName; // Буффер для пути
	ofn.nMaxFile = MAX_PATH; // Максимальный размер пути
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY; // Флаги - открыть в проводнике, отображаемый файл должен существовать, скрыть файлы только для чтения
	ofn.lpstrDefExt = extension.c_str(); // Расширение для сохранения по умолчанию

	GetSaveFileName(&ofn); // Вызываем функцию из WinAPI
	return ofn.lpstrFile; // Возвращаем путь до файла
}

// Структура для вершины дерева. Нужна для создания дерева Хаффмана
struct Node {
	Node* L, *R; // Указатели на детей
	uint8_t val; // Байт в этой вершине
	Node() { // Конструктор по умолчанию
		L = R = nullptr;
		val = 0;
	}
	Node(uint8_t val) { // Конструктор для листа - фактически задан какой-то байт
		L = R = nullptr;
		this->val = val;
	}
	bool is_leaf() { // Проверка на лист
		return L == R && R == nullptr;
	}
};

// Функция для генерации дерева по вектору байтов
Node* generate_tree(std::vector<uint8_t>& file_buff) {
	// Вектор, в котором мы будем хранить частоту вхождения каждого байта 
	std::vector<int> freq(256, 0); // Всего существует 2^8=256 различных байтов
	for (size_t i = 0; i < file_buff.size(); ++i) {
		++freq[file_buff[i]]; // Проходимся по байтам файла и изменяем частоту
	}

	// Set для того, чтобы получать вершины с минимальной частотой вхождения
	// Каждый элемент представлет из себя pair<pair<int, int>, Node*>. Фактически мы храним тройки некоторых значений:
	// Первое - частота вхождения текущей вершины
	// Второе - "время добавления". Чем раньше мы добавили вершину в set, тем больше будет это значение. Я добавил его
	// для борьбы с undefined behavior из-за того, что указатель каждый раз имеет новый адрес
	// Третье - указатель на вершину
	std::set<std::pair<std::pair<int, int>, Node*>> prior;
	for (int i = 0; i < 256; ++i) {
		// Если данный байт встречается хотя бы один раз, то добавим его в set.
		if (freq[i] != 0)
			prior.insert(std::make_pair(std::make_pair(freq[i], -i), new Node(i))); // заносим в set ((частота, -время добавления), указатель на вершину)
	}

	int _time = 256; // Заведем переменную с временем. Изначально это 256, так как это точно больше, чем максимальное время из уже добавленных вершин
	while (prior.size() > 1) { // Пока set состоит из >= 2 элементов
		// Достаем первые два элемента и удаляем их
		std::pair<std::pair<int, int>, Node*> top1 = *prior.begin();
		prior.erase(prior.begin());
		std::pair<std::pair<int, int>, Node*> top2 = *prior.begin();
		prior.erase(prior.begin());

		// Создаем новую вершину, которая объединяет в себе две только что взятые
		Node* new_node = new Node();
		new_node->L = top1.second;
		new_node->R = top2.second;
		// Добавлем эту вершину и сопуствующую ей информацию (частота и время) в set
		prior.insert(std::make_pair(std::make_pair(top1.first.first + top2.first.first, -_time), new_node));
		++_time; // Увеличим время
	}
	return prior.begin()->second; // Возвращаем указатель на единственную оставшуюся вершину. Фактически это корень дерева Хаффмана
}

// Функция для генерации промежуточной таблицы Хаффмана. Таблица представляет из себя набор кодов размера 256 (для каждого байта)
std::vector<BitVector> generate_table(Node* p) { // В аргументах принимаем указатель на корень дерева
	std::vector<BitVector> table(256); // Создадим саму таблицу для каждого возможного байта
	BitVector curr; // Текущий код
	// Создадим функцию для dfs-обхода дерева
	auto dfs = [&](Node* p, auto f) { // Это называется лямбда-функция. Это просто функция внутри другой функции. Для удобства можно 
		// воспринимать ее как void dfs(Node* p). auto f и [&] нужны для встраивания ее в generate_table.
		if (p->is_leaf()) { // Если текущая вершина-лист, то занесем в таблицу полученный код, сохраненный в curr
			table[p->val] = curr; // val - байт, соответствующий этой вершине
			return;
		}
		if (p->L != nullptr) { // Пойдем в левого сына, если он есть
			curr.push_back(0); // Добавим 0 в текущий код, так как спуск влево=добавление 0
			f(p->L,f); // Рекурсия
			curr.pop_back(); // Удалим 0 из кода
		}
		if (p->R != nullptr) { // Аналогично для правого сына
			curr.push_back(1);
			f(p->R, f);
			curr.pop_back();
		}
	};

	dfs(p, dfs); // Вызываем обход дерева и возвращаем полученную таблицу
	return table;
}

// Основная функция для генерации таблицы из вектора байтов файла
std::vector<std::pair<uint8_t, BitVector>> generate_table(std::vector<uint8_t>& file_buff) {
	Node* p = generate_tree(file_buff); // Сначала получим указатель на корень дерева Хаффмана
	std::vector<BitVector> table = generate_table(p); // Теперь по нему сформируем таблицу. Заметим, что в этой таблице также есть неиспользуемые байты,
	// им соответствует пустой код. Отфильтруем их и занесем нормальную таблицу в вектор filtered. Теперь таблица будет представлять из себя набор
	// пар (байт, код).
	std::vector<std::pair<uint8_t, BitVector>> filtered;
	for (int i = 0; i < 256; ++i) { // Пройдемся по всем байтам, и если код для i-ого байта не пустой, то занесем эту пару в таблицу
		if (table[i].empty()) continue;
		filtered.push_back(std::make_pair(i, table[i]));
	}
	return filtered; // Вернем таблицу
}

// Преобразование набора битов с помощью гамма-функции Эллиса
BitVector to_gamma_code(BitVector& bits) {
	BitVector added = bits;
	added.push_front(1); // Добавим в начало 1
	int sz = (int)added.size() - 1; // Добавим в начало bits.size() - 1 (с учетом новой единицы) нулей
	for (int i = 0; i < sz; ++i) {
		added.push_front(0);
	}
	return added; // Это и есть значение гаммиа-функции Эллиаса
}

// Функция для записи набора битов bits в файл. В качестве аргументов принимаем ссылку на поток для записи (file),
// набор битов (bits), ссылку на текущий записываемый байт (curr_byte) и текущую позицию в этом байте для записи (curr_pos)
void write(std::ofstream& file, BitVector& bits, uint8_t& curr_byte, int& curr_pos) {
	int written = 0; // Количество записанных битов из набора bits
	while (bits.size() != written) { // Пока есть неиспользованные биты
		while (curr_pos < 8 && bits.size() != written) { // Пока мы не вышли за пределы текущего байта И остались еще биты для записи
			if (bits[written] == 1) // Если значение бита равно 1
				curr_byte |= 1LL << (7 - curr_pos); // Запишем его на позицию curr_pos в текущем байте
			++curr_pos; // Передвинем позицию для записи на 1
			++written; // Увеличим количество записанных битов
		}
		if (curr_pos == 8) { // Если мы закончили запись в текущий байт
			file.write((char*)&curr_byte, sizeof(uint8_t)); // Запишем этот байт в байл
			curr_pos = 0; // Обнулим текущий байт и позицию для записи в него
			curr_byte = 0;
		} else break; // Иначе выйдем из цикла, потому что биты закончились
	}
}

// Функция для получения набора битов из числа. Передаем число в виде unsigned long long и количество битов, в котором нужно уместить это число
BitVector get_bits(size_t n, int bits_count) {
	BitVector vec; // Заводим "вектор битов"
	for (int i = 0; i < bits_count; ++i) { // Проходимся по всем указанным битам
		vec.push_back((n >> (bits_count - 1 - i)) & 1); // Добавляем их в конец набора битов vec
	}
	return vec; // vec - представление числа n в bits_count битах
}

// Функция для записи таблицы Хаффмана в файл. Передаем таблицу Хаффмана по ссылке, ссылки на текущий байт, позицию для записи и поток файла
void write_table(std::vector<std::pair<uint8_t, BitVector>>& huffman_table, uint8_t& curr_byte, int& curr_pos, std::ofstream& stream) {
	// Значала запишем в начало файла количество строк в таблице с помощью 32 битов. Этого количества гарантированно хватит
	BitVector bits = get_bits(huffman_table.size(), 32);
	write(stream, bits, curr_byte, curr_pos);
	// Проходим по всем строкам в таблице
	for (int i = 0; i < huffman_table.size(); ++i) {
		bits = get_bits(huffman_table[i].first, 8); // Сначала записываем в таблицу 8 битов для хранения исходного байта
		write(stream, bits, curr_byte, curr_pos);
		bits = to_gamma_code(huffman_table[i].second); // Потом записываем биты для шифрования. Поскольку их количество заранее неизвестно,
		// то запишем в файл гамма-функцию Эллиаса этих битов. Если нет понимания, зачем - применение этой функции хорошо описано на Вики ИТМО
		write(stream, bits, curr_byte, curr_pos);
	}
}

// Функция для архивации файла и его сохранения
// Аргументы - байты из файла, таблица Хаффмана и текущий байт и позиция в нем
void write_zip(std::vector<uint8_t>& file_buff, std::vector<std::pair<uint8_t, BitVector>>& huffman_table, uint8_t& curr_byte, int& curr_pos) {
	// Создаем новый поток для записи. Флаг trunc означает перезапись существующего файла, binary - побайтовая запись
	// В качестве пути для сохранения передаем значение функции get_file_save_path. Наши архивы имеют расширение .bruh.
	std::ofstream stream(get_file_save_path("bruh"), std::fstream::trunc | std::fstream::binary);

	write_table(huffman_table, curr_byte, curr_pos, stream); // Сначала запишем в файл таблицу Хаффмана

	std::vector<std::pair<uint8_t, BitVector*>> nums; // Вектор пар для быстрого поиска байта в таблице.
	// Добавим в него все строки из таблице в формате (байт, указатель на код). Отсортировав его, мы сможем быстро получать значение кода с помощью
	// бинарного поиска по этому вектору
	for (int i = 0; i < huffman_table.size(); ++i) {
		nums.push_back(std::make_pair(huffman_table[i].first, &huffman_table[i].second));
	}
	std::sort(nums.begin(), nums.end()); // Сортируем его
	
	BitVector bits = get_bits(file_buff.size(), 32);
	write(stream, bits, curr_byte, curr_pos); // Запишем в файл количество зашифрованных байтов. Хранить это количество будем с помощью 32 битов.
	for (int i = 0; i < file_buff.size(); ++i) { // Проходимся по байтам из файла
		auto it = std::lower_bound(nums.begin(), nums.end(), std::make_pair(file_buff[i], (BitVector*)nullptr)); // С помощью lower_bound
		// находим интересующий нас байт и его код в векторе nums. (BitVector*)nullptr позволяет не пропустить элемент при сравнении, потому что
		// любой указатель будет больше, чем nullptr.
		write(stream, *it->second, curr_byte, curr_pos); // Запишем в файл биты для шифровки текущего байта
	}
	if (curr_pos != 0) { // Если последний байт не пустой, значит нам нужно дописать его в файл
		stream.write((char*)&curr_byte, sizeof(uint8_t)); // Допишем последний байт в файл-архив
	}
	stream.close(); // Закроем поток для записи
}

// Функция для получения следующего бита из файла. Получаем ссылки на текущий байт, позицию в нем и поток для чтения
bool get_next_bit(uint8_t& curr_byte, int& curr_pos, std::ifstream& stream) {
	bool val = (curr_byte >> (7 - curr_pos)) & 1; // Получим значение бита на позиции curr_pos в текущем рассматриваемом байте
	++curr_pos; // Передвинем текущую позицию
	if (curr_pos == 8) { // Если мы вышли за границу байта
		curr_pos = 0; // Обнулим позицию
		stream.read((char*)&curr_byte, sizeof(curr_byte)); // Считаем новый байт в curr_byte
	}
	return val; // Вернем значение считанного бита
}

// Получение числа из n следующих битов в текущем файле. Аргументы - как обычно + количество битов
size_t get_from_n_bits(int n, uint8_t& curr_byte, int& curr_pos, std::ifstream& stream) {
	size_t val = 0; // Переменная для записи. Гарантируем сами себе, что n <= 64
	for (int i = 0; i < n; ++i) { // Считываем n битов
		val |= get_next_bit(curr_byte, curr_pos, stream) * (1LL << (n - i - 1)); // с помощью битового ИЛИ добавляем новый бит на соответствующую позицию в числе
	}
	return val;
}

// Получение набора битов из их гамма-кода Эллиса
// Аргументы - как обычно
BitVector from_gamma_code(uint8_t& curr_byte, int& curr_pos, std::ifstream& stream) {
	BitVector bits; // "вектор битов"
	int n = 0;
	// Считаем количество нулей до следующего единичного бита
	while (get_next_bit(curr_byte, curr_pos, stream) == 0) {
		++n;
	}
	// Записываем в bits ровно n следующих битов из файла
	for (int j = 0; j < n; ++j) {
		bits.push_back(get_next_bit(curr_byte, curr_pos, stream));
	}
	return bits;
}

// Функция для считывания таблицы Хаффмана из файла (таблица в сжатом виде, то есть без неиспользуемых байтов)
std::vector<std::pair<uint8_t, BitVector>> read_table(uint8_t& curr_byte, int& curr_pos, std::ifstream& stream) {
	int table_size = get_from_n_bits(32, curr_byte, curr_pos, stream); // Получим количество строк в таблице. Мы записывали это как 32 бита
	std::vector<std::pair<uint8_t, BitVector>> table; // Вектор-таблица
	for (int i = 0; i < table_size; ++i) {
		uint8_t c = get_from_n_bits(8, curr_byte, curr_pos, stream); // Сначала считываем 8 битов - исходный байт
		table.push_back(std::make_pair(c, from_gamma_code(curr_byte, curr_pos, stream))); // Считываем его шифр с помощью гамма-кода Эллиаса,
		// добавляем эту пару как новую строку в таблицу
	}
	return table; // Возвращаем полученную таблицу
}

// Функция для считывания заархивировнного файла и его декодирования
std::pair<std::vector<uint8_t>, std::string> read_unzip(uint8_t& curr_byte, int& curr_pos, std::ifstream& stream) {
	std::vector<std::pair<uint8_t, BitVector>> table = read_table(curr_byte, curr_pos, stream); // Сначала получим таблицу Хаффмана
	
	std::vector<std::pair<std::pair<size_t, int>, uint8_t>> nums; // Сделаем вектор для быстрой расшифровки байтов. Работает аналогично с write_zip
	for (int i = 0; i < table.size(); ++i) {
		// Элементы вектора представлены в виде ((байт в зашифрованном виде, количество битов в коде), оригинальное значение байта)
		nums.push_back(std::make_pair(std::make_pair(table[i].second.value(), table[i].second.size()), table[i].first));
	}
	std::sort(nums.begin(), nums.end());

	std::vector<uint8_t> buffer; // В этом векторе будут храниться все байты файла
	int sz = get_from_n_bits(32, curr_byte, curr_pos, stream); // Считаем количество зашифрованных байтов. Это ровно 32 бита

	std::string extension = ""; // Переменная, в которой хранится расширение исходного файла
	int zeros = 0; // Количество нулевых битов. Как только мы получим 2 нулевых бита, то чтение расширения завершится
	size_t curr_bits = 0; // Текущий считанный код
	int curr_sz = 0; // Количество считанных битов
	while (buffer.size() + extension.size() + 2 != sz) { // Пока суммарное количество полученных байтов не равно sz
		bool bit = get_next_bit(curr_byte, curr_pos, stream); // Получаем новых бит и добавляем его к curr_bits
		curr_bits |= bit * (1LL << (63 - curr_sz));
		++curr_sz; // Увеличиваем количество считанных битов
		auto it = std::lower_bound(nums.begin(), nums.end(), std::make_pair(std::make_pair(curr_bits, curr_sz), (uint8_t)0)); // Ищем байт с таким кодом
		if (it == nums.end() || it->first.first != curr_bits || it->first.second != curr_sz) // Найденный код отличается
			continue; // Идем дальше
		curr_bits = 0; // Иначе, мы можем расшифровать очередной байт (он равен it->second), обнулим значение curr_bits и curr_sz
		curr_sz = 0;
		if (zeros >= 2) // Если мы уже сгенерировали расширение (количество считанных нулей >= 2), то добавим новый байт в вектор buffer
			buffer.push_back(it->second);
		else { // Значит, мы заполняем расширение
			if (it->second != 0) // Если получили не 0, то добавим новый символ в строку extension
				extension.push_back(it->second);
			else // Иначе, увеличим количество найденных нулей
				++zeros;
		}
	}
	return std::make_pair(buffer, extension); // Вернем в качестве результата пару (байты файла, расширение)
}

// Функция для проверки возможности открытия файла
bool check_file(std::string path) {
	std::fstream stream(path, std::fstream::in); // Открываем поток
	bool is_ok = stream.is_open() && stream.good(); // Если поток открыт и он "хороший"), то файл валиден
	if (is_ok) // Закроем поток, если он открыт
		stream.close();
	return is_ok; // Вернем инфорацию об успешности открытия
}

void run(std::string operation, std::string file_path) {
	// Пытаемся открыть данный файл
	if (!check_file(file_path)) {
		std::cout << "Invalid file path\n";
		return;
	}

	// is-else конструкция для разбора случаев с каждой из возможных операция
	if (operation == "zip") { // архивация
		std::vector<uint8_t> file_buff = get_file_bytes(file_path); // Получаем все байты из файла

		// Блок для добавления расширения файла в начало его байтов.
		std::reverse(file_buff.begin(), file_buff.end()); // Сначала переворачиваем вектор байтов
		file_buff.push_back(0); // Добавим нулевой байт в качестве разделителя
		size_t last_backslash = file_path.rfind('\\'); // Найдем последний индекс вхождения \ в строку file_path
		size_t ext_index = file_path.rfind('.'); // Найдем последний индекс вхождения . в строку file_path
		if (ext_index > last_backslash) { // Если последняя точка находится правее последнего \, то файл имеет расширение
			std::string ext = file_path.substr(ext_index + 1); // Строка, которая хранит в себе расширение файла
			for (int i = ext.size() - 1; i >= 0; --i) { // Добавим все ее символы в байты в файла
				file_buff.push_back((uint8_t)ext[i]);
			}
		}
		file_buff.push_back(0); // Добавим второй разделительный нулевой байт
		std::reverse(file_buff.begin(), file_buff.end()); // Развернем вектор байтов. Поскольку мы записывали расширение в конец ветора справа-налево,
		// то теперь он имеет вид 0[байты расширения]0[первоначальные байты файла]
		
		std::vector<std::pair<uint8_t, BitVector>> huffman_table = generate_table(file_buff); // Генерируем таблицу Хаффмана

		uint8_t curr_byte = 0; // Заводим переменные curr_byte и curr_pos для побитовой записи в файл
		int curr_pos = 0;
		write_zip(file_buff, huffman_table, curr_byte, curr_pos); // Архивируем наш файл с помощь функции write_zip
		std::cout << "Done\n";
	} else if (operation == "unzip") { // Деархивация
		uint8_t curr_byte = 0; // Заводим переменные curr_byte и curr_pos для побитового чтения из файла
		int curr_pos = 0;
		std::ifstream stream(file_path, std::fstream::binary); // Поток для чтения
		stream.read((char*)&curr_byte, sizeof(curr_byte)); // Считаем первый байт
		std::pair<std::vector<uint8_t>, std::string> file_unzipped = read_unzip(curr_byte, curr_pos, stream); // С помощью функции read_unzip деархивируем файл
		stream.close(); // Закроем поток для чтения
		std::ofstream out(get_file_save_path(file_unzipped.second), std::ios::binary); // Открываем поток для сохранения по пути get_file_save_path,
		// в качестве целевого расширения укажем file_unzipped.second
		out.write((char*)&file_unzipped.first[0], file_unzipped.first.size()); // Запишем все байты в поток
		out.close(); // Закроем его
		std::cout << "Done\n";
	} else if (operation == "gen") { // Генерации таблицы Хаффмана
		// Получаем байты из файла, для которого хотим сгенерить таблицу
		std::vector<uint8_t> file_buff = get_file_bytes(file_path);

		std::vector<std::pair<uint8_t, BitVector>> huffman_table = generate_table(file_buff); // Генерируем таблицу

		std::cout << "Table for encrypting bytes in a file:\n"; // Выводим ее в красивом формате
		for (int i = 0; i < huffman_table.size(); ++i) {
			std::cout << (int)huffman_table[i].first << " <-> ";
			for (int j = 0; j < huffman_table[i].second.size(); ++j) {
				std::cout << huffman_table[i].second[j];
			}
			std::cout << '\n';
		}
		std::cout << "Do you want to save it to a file? (y/n)\n";
		char c;
		std::cin >> c; // Проверяем, хочет ли пользователь сохранить ее в файл
		if (c == 'y' || (c ^ 'y') == 32) { // c == 'y' || c == 'Y'
			std::ofstream stream(get_file_save_path("bruhtb"), std::fstream::trunc | std::fstream::binary); // Поток для записи таблиц, расширение - bruhtb
			uint8_t curr_byte = 0; // Заводим переменные curr_byte и curr_pos для побитовой записи в файл
			int curr_pos = 0;
			write_table(huffman_table, curr_byte, curr_pos, stream); // Записываем только таблицу
			if (curr_pos != 0) { // Если последний байт не пустой, то запишем и его
				stream.write((char*)&curr_byte, sizeof(uint8_t));
			}
			stream.close(); // Закроем поток
			std::cout << "Done\n";
		} else {
			return;
		}
	} else if (operation == "regen") { // Получение таблицы Хаффмана из файла
		uint8_t curr_byte = 0; // Заводим переменные curr_byte и curr_pos для побитового чтения из файла
		int curr_pos = 0;
		std::ifstream stream(file_path, std::fstream::binary); // Потко для чтения
		stream.read((char*)&curr_byte, sizeof(curr_byte)); // Считываем первый байт
		std::vector<std::pair<uint8_t, BitVector>> huffman_table = read_table(curr_byte, curr_pos, stream); // Считываем таблицу Хаффмана
		std::cout << "Unzipped table for encrypting bytes in a file:\n"; // Выводим ее в красивом формате
		for (int i = 0; i < huffman_table.size(); ++i) {
			std::cout << (int)huffman_table[i].first << " <-> ";
			for (int j = 0; j < huffman_table[i].second.size(); ++j) {
				std::cout << huffman_table[i].second[j];
			}
			std::cout << '\n';
		}
		stream.close(); // Закрываем поток
	} else { // Неизвестная операция
		std::cout << "Invalid operation type\n";
		return;
	}
}

// Точка входа в программу. Аргументы argc и argv нужны для запуска через консоль. argc - количество аргументов, argv - сами аргументы в виде строк.
// Если программа запускалась не через консоль/без аргументов, то argc = 1, argv - путь до исполняемого файла
int main(int argc, char* argv[]) {
	if (argc == 3) { // Если нам дали 2 доп. аргумента, то выполним необходимую операцию
		run(argv[1], argv[2]);
	}
	if (argc != 3 && argc > 1) { // Если количество доп. аргументов отличается от 2, то выведем информацию об ошибке
		std::cout << "Invalid args provided\n";
		return 0;
	}

	while (true) { // Бесконечный цикл
		std::string operation, path; // Получим операцию и путь до файла
		std::cin >> operation;
		if (operation == "?") { // Если операция это "?", то выведем "инструкцию" для работы с программой
			std::cout << "zip - file archiving using the Huffman algorithm\nunzip - file dearchiving\ngen - table generation for coding\nregen - retrieving a table from a file\n";
			continue;
		}
		std::cin >> path;
		run(operation, path); // Запустим основуню функцию
	}
}