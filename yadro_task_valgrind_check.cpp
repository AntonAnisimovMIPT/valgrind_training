#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <limits>
#include <optional>
#include <chrono>

struct Configurations {

    double time_reading;
    double time_writing;
    double time_shifting;
    double time_rewinding;
    bool is_configurated = false;

};

struct Dirs_Files {
    std::string directory;
    std::string filename;
};

struct RAM_Element {
    std::optional<uint32_t> value;
    std::size_t index;
};

auto configurate (const std::string& conf_file_name, Configurations& configs) {
    std::ifstream config_file(conf_file_name);
    config_file >> configs.time_reading 
                >> configs.time_writing 
                >> configs.time_shifting 
                >> configs.time_rewinding;
    configs.is_configurated = true;
}

std::ostream& operator<<(std::ostream& os, const std::optional<uint32_t>& value) {
    if (value.has_value()) {

        os << value.value();

    } else {

        os << "empty"; 

    }
    return os;
}

class RAM_emu {
    
public:
    RAM_emu(std::size_t M) : RAM_size(M) {};

    void add_element(std::optional<uint32_t> added) { 
        if (std::size(storage) < get_number_of_cells()) {
            
            storage.push_back(added);

        }
        else {
            
            // при переполнении памяти она очищается, чтобы можно было снова добавлять элементы
            storage.clear();
            storage.push_back(added);
            
        }
        
    }

    std::size_t get_number_of_cells() const {

        return std::ceil(RAM_size / static_cast<std::size_t>(sizeof(std::uint32_t)));

    }

    void print_storage() const {

        if (storage.empty()) {

            std::cout << "RAM is empty!\n";

        }
        else {

            std::cout << "current elements in storage:\n";
            for (auto i : storage) {

                std::cout << i << "\n";

            }

        }
            
    }

    RAM_Element get_min_element() const {
        
        if (!storage.empty()) {

            auto min_element_it = std::min_element(storage.begin(), storage.end(), 
            // для работы с std::optional для функции сравнения нужно написать собственный компаратор
            [](const auto& a, const auto& b) {

                if (!a) return false; // если a пуст, b должен быть меньше
                if (!b) return true; // если b пуст, a должен быть меньше
                return *a < *b;

            });

            auto min_index = std::distance(storage.begin(), min_element_it);
            RAM_Element max_element = { .value = *min_element_it, 
                                        .index = min_index };
            return max_element;     

        } else {

            std::cout << "Storage is empty! The operation of finding the maximum element is incorrect.\n";

        }

    }

    std::optional<uint32_t> get_value_in_RAM(std::size_t index) const {
        if (index >= storage.size()) {

            std::cout << "incorrect index in RAM\n";

        }

        auto it = std::next(storage.begin(), index);
        return *it;
    }

    std::optional<uint32_t> get_value_in_RAM() const {

        return storage.back();

    }

    void set_element_in_RAM(const RAM_Element& ram_elem) {
        if (!storage.empty()) {

            storage[ram_elem.index] = ram_elem.value;

        }
        else {

            std::cout << "storage is empty!\n";

        }
        
    }

    void clear_RAM() {

        storage.clear();

    }

    void sort_storage() {

        std::sort(storage.begin(), storage.end(), std::less<std::optional<uint32_t>>());

    }

private:
    std::vector<std::optional<uint32_t>> storage;
    std::size_t RAM_size;
    
};

class Tape_emu {
    public:
        Tape_emu(const std::string& input_name, RAM_emu& ram_emu, Configurations& config_dt, const std::string& conf_file)
        : input_file(input_name), RAM(ram_emu), configurations_data(config_dt), position(0), elapsed_time(0) {

        if (!config_dt.is_configurated) {
            configurate(conf_file, configurations_data);
        }
        
    }

    void set_output_files(std::vector<Dirs_Files> output_names) {

        for (const auto& file_info : output_names) {
            std::filesystem::create_directories(file_info.directory);
            output_files.emplace_back(file_info.directory + "/" + file_info.filename);
        }

    }
    
    void read() {
        
        std::string reading_line;

        if (std::getline(input_file, reading_line)) {
            
            auto digit_line = std::stoul(reading_line);
            RAM.add_element(digit_line);

            elapsed_time += configurations_data.time_reading;
            ++position; // т.к. сдвиг действительно есть из-за std::getline
            elapsed_time += configurations_data.time_shifting; 

        } else {
            
            // если дошли до конца, то элемент в оперативной памяти меняется на std::nullopt
            RAM.add_element(std::nullopt);

        }
        
    }

    void read(std::size_t index) {

        std::string reading_line;
        
        if (std::getline(input_file, reading_line)) {

            auto digit_line = std::stoul(reading_line);
            RAM_Element ram_elem{ .value = digit_line, .index = index};
            RAM.set_element_in_RAM(ram_elem);
            
            elapsed_time += configurations_data.time_reading;
            ++position; // т.к. сдвиг действительно есть из-за std::getline
            elapsed_time += configurations_data.time_shifting; 

        } 
        else {

            // если дошли до конца, то элемент в оперативной памяти меняется на std::nullopt
            RAM_Element ram_elem{ .value = std::nullopt, .index = index}; 
            RAM.set_element_in_RAM(ram_elem);

        }
    }

    void write(std::size_t output_file_index) {

        if (RAM.get_value_in_RAM() != std::nullopt) {

            output_files[output_file_index] << RAM.get_value_in_RAM() << "\n";

        }

    }

    void write(std::size_t index_in_RAM, std::size_t output_file_index) {

        if (RAM.get_value_in_RAM(index_in_RAM) != std::nullopt) {

            output_files[output_file_index] << RAM.get_value_in_RAM(index_in_RAM) << "\n";

        }

    }

    void rewind() {
        
        input_file.clear();
        // после перемотки считывающая головка устанавливается на начало ленты
        input_file.seekg(0, std::ios::beg);
        elapsed_time += configurations_data.time_rewinding;
        position = 0; 

    }
        
    void shift_left() {
            if (position > 0) {

                --position;
                input_file.seekg(-static_cast<std::streamoff>(sizeof(std::uint32_t)), std::ios_base::cur);
                elapsed_time += configurations_data.time_shifting;

            } else {
                std::cout << "It's impossible to move to left, because the beginning of the file has already been reached.\n";
            }
        }
        
    void shift_right() {
            
            // для смещения вправо немного сложнее логика проверок, т.к. заранее мы не знаем размер ленты
            
            auto current_pos = input_file.tellg();

            // чтобы определить размер размер файла
            input_file.seekg(0, std::ios::end);
            auto file_size = input_file.tellg();

            input_file.seekg(current_pos); 

            
            if (current_pos + sizeof(std::uint32_t) < file_size) {

                input_file.seekg(static_cast<std::streamoff>(sizeof(std::uint32_t)), std::ios_base::cur);
                ++position;
                elapsed_time += configurations_data.time_shifting;

                }
                else {
                    std::cout << "It's impossible to move to right, because the end of the file has already been reached.\n";
                }
        }

    double get_elapsed_time() const {

            return elapsed_time;
            
        }
        
    std::size_t get_position() const {

            return position;

        }
        
    std::size_t get_number_output_files() const {
            
            return output_files.size();

        }
        
    std::size_t count_lines() {

            std::streampos current_pos = input_file.tellg();
            std::size_t line_count = 0;
            std::string line;

            while (std::getline(input_file, line)) {

                ++line_count;

            }

            input_file.clear();
            input_file.seekg(current_pos); 
            return line_count;
        }

private:

    double elapsed_time;
    std::size_t position;
    std::ifstream input_file;
    RAM_emu& RAM; 
    Configurations& configurations_data;
    std::vector<std::ofstream> output_files;
    
};

void create_directory(const std::string& directory) {
    
        if (std::filesystem::exists(directory)) { 
            std::filesystem::remove_all(directory); 
        }

        std::filesystem::create_directory(directory); 
}

// данная функция создает в папке tmp нужное количество временных файлов в отсортированном виде
void prepare_files(Tape_emu& tape, RAM_emu& ram, std::string target_directory) {

    if (!std::filesystem::exists(target_directory))
        std::filesystem::create_directory(target_directory); 

    auto number_files = std::ceil(static_cast<double>(tape.count_lines())/static_cast<double>(ram.get_number_of_cells()));
    auto number_values = tape.count_lines();

    for (size_t file_index = 0; file_index < number_files; file_index++) {
       
        if (file_index != number_files - 1)
        {
            for (size_t i = 0; i < ram.get_number_of_cells(); i++) {
                tape.read();
            }

            ram.sort_storage();

            for (size_t i = 0; i < ram.get_number_of_cells(); i++) {
                tape.write(i, file_index);
            }
        }
        else {

            auto remain_indexes = number_values - ram.get_number_of_cells() * file_index;
            for (size_t i = 0; i < remain_indexes; i++) {
                tape.read();
            }

            ram.sort_storage();

            for (size_t i = 0; i < remain_indexes; i++) {
                tape.write(i, file_index);
            }
        }

    }
    tape.rewind(); 
    
}

std::vector<Dirs_Files> generate_files_paths(const std::string& general_directory, Tape_emu& tape, RAM_emu& ram) {
    
    std::vector<Dirs_Files> files_paths;
    auto number_files = std::ceil(static_cast<double>(tape.count_lines())/static_cast<double>(ram.get_number_of_cells()));
    for (int i = 0; i < number_files; ++i) {

        files_paths.push_back({general_directory, std::to_string(i)+".txt"});

    }
    return files_paths;
}

class Sorter {
public:
    Sorter(Tape_emu& input_tape, RAM_emu& ram, Configurations& config_dt, const std::string& conf_file) : tape_emu(input_tape), ram_emu(ram) {
            
            out_path = { .directory = ".", .filename = "output.txt"};
            std::vector<Dirs_Files> output{out_path};
            
            auto tmp_files = generate_files_paths("tmp", tape_emu, ram_emu);
            for (auto files : tmp_files) {
                tape_emus.emplace_back(files.filename, ram_emu, config_dt, conf_file); 
            
        }
            
        for (size_t index = 0; index < tape_emus.size(); index++) {

            tape_emus[index].set_output_files(output);

        }
    };

    void sorting() {

    ram_emu.clear_RAM();
    auto upper_bound_iterations = tape_emu.count_lines();

    for (size_t i = 0; i < upper_bound_iterations; i++) {
        // для первой итерации алгоритм имеет особенность ввиду разницы в работе перегруженных функций read()
        if (i == 0) {
        
            for (size_t index = 0; index < tape_emus.size(); index++) {
                    
                tape_emus[index].read();

            }
                
            auto min_value = ram_emu.get_min_element().value;
            auto min_element_index = ram_emu.get_min_element().index;

            // по умолчанию записывающая лента - первая лента
            tape_emus[0].write(min_element_index, 0);
            tape_emus[min_element_index].read(min_element_index); 
            
        }

        else {
            for (size_t index = 0; index < tape_emus.size(); index++) {
            
                auto min_value = ram_emu.get_min_element().value;
                auto min_element_index = ram_emu.get_min_element().index;
                    
                // по умолчанию записывающая лента - первая лента
                tape_emus[0].write(min_element_index, 0);
                tape_emus[min_element_index].read(min_element_index); 
                    
            }
        }
    }
}
private:
    std::vector<std::ofstream> tmp_files;
    std::vector<Tape_emu> tape_emus;
    RAM_emu& ram_emu;
    Tape_emu& tape_emu;
    Dirs_Files out_path;
};

int main(int argc, char** argv) {

    std::string input = "input.txt";
    RAM_emu ram_emu(32); 
    Configurations confs;
    auto conffile = "configurations.txt";

    Tape_emu tape(input, ram_emu, confs, conffile);

    auto tmps_paths = generate_files_paths("tmp", tape, ram_emu);
    tape.set_output_files(tmps_paths);

    std::string action = argv[1];
    if (action == "prepare") {

        prepare_files(tape, ram_emu, "tmp");

    }

    else if (action == "sort") {
        
        Sorter srt(tape, ram_emu, confs, conffile);
        srt.sorting();

    }

}