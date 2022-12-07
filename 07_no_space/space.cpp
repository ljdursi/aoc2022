#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <algorithm>
#include <numeric>
#include <cassert>

#include "range/v3/all.hpp"
namespace rv = ranges::views;

class File {
        std::string name;
        size_t size;

    public:
        File(std::string name, size_t size) : name(name), size(size) {};

        std::string get_name() const {
            return name;
        }

        size_t total_size() const {
            return size;
        }

        friend std::ostream& operator<<(std::ostream& os, const File& file) {
            os << " - " << file.name << " " << "(file, size=" << file.size << ")";
            return os;
        }
};

class Directory {
    private:
        size_t size;
        std::string name;
        std::map<std::string, File> files;
        std::map<std::string, Directory> directories;
        bool size_needs_update;
        Directory *parent;

    public:
        Directory(std::string name, Directory *parent=nullptr) : name(name), size(0), parent(parent), size_needs_update(false) {};
        size_t total_size() const;

        std::string full_name() const {
            std::vector<std::string> path;
            const Directory *dir = this;
            while (dir) {
                path.push_back(dir->name);
                dir = dir->parent;
            }

            std::reverse(path.begin(), path.end());

            std::string result = path[0];
            for (size_t i = 1; i < path.size()-1; i++) {
                result += path[i] + "/";
            }
            if (path.size() > 1)
                result += path[path.size()-1];

            return result;
        }

        Directory *cd(std::string directory_name) {
            if (directory_name == ".") {
                return this;
            }

            if (directory_name == "..") {
                if (parent)
                    return parent;
                else
                    return this;
            }

            auto result = directories.find(directory_name);
            if (result == directories.end())
                return this;

            return &result->second;
        }

        void calculate_size() {
            if (!size_needs_update) {
                return;
            }

            size = 0;
            for (auto&& [file_name, file] : files) {
                size += file.total_size();
            }

            for (auto&& [dir_name, dir] : directories) {
                size += dir.total_size();
            }

            size_needs_update = false;
        };

        size_t total_size() { 
            if (size_needs_update) {
                calculate_size();
            }
            return size;
        }

        void add_file(std::string file_name, size_t size) {
            if (files.find(file_name) == files.end()) {
                files.insert(std::make_pair(file_name, File(file_name, size)));
                size_needs_update = true;
            }
        };

        void add_dir(std::string directory_name) {
            if (directory_name == ".") {
                return;
            }

            if (directory_name == "..") {
                if (!parent) {
                    parent = new Directory("parent");
                    size_needs_update = true;
                } 
                return;
            }

            if (directories.find(directory_name) == directories.end()) {
                directories.insert(std::make_pair(directory_name, Directory(directory_name, this)));
                size_needs_update = true;
                return;
            }
        };

        std::string to_string(std::string prefix="") {
            std::string sub_prefix = prefix + "  ";
            std::stringstream ss;

            ss << prefix << "- " << name << " (dir)" << std::endl;
            for (auto& file : files) 
                ss << sub_prefix << file.second << std::endl;

            for (auto& directory : directories)
                ss << directory.second.to_string(sub_prefix);

            return ss.str();
        }

        std::vector<std::pair<std::string, std::size_t>> directory_sizes() {
            std::vector<std::pair<std::string, std::size_t>> result;

            result.push_back(std::make_pair(full_name(), total_size()));
            for (auto&& [dir_name, dir]: directories) {
                auto subdirs = dir.directory_sizes();
                std::copy(subdirs.begin(), subdirs.end(), std::back_inserter(result));
            }

            return result;
        }
};

class TraverseDirectoryTree {
    private:
        Directory root;
        Directory *cwd;

    public:
        Directory *get_root() {
            return &root;
        }

        TraverseDirectoryTree(std::vector<std::string> terminal) : root("/"), cwd(&root) {
            size_t linenum = 0;
            size_t n_lines = terminal.size();
            while (linenum < n_lines) {
                auto& command = terminal[linenum++];

                assert(command.size() > 3);
                assert(command[0] == '$');

                std::stringstream ss(command.substr(2));
                std::string command_name;
                ss >> command_name;

                if (command_name == "cd") {
                    std::string directory_name;
                    ss >> directory_name;

                    if (directory_name == "/") {
                        cwd = &root;
                    } else {
                        cwd->add_dir(directory_name);
                        cwd = cwd->cd(directory_name);
                    }
                } else if (command_name == "ls") {
                    while (linenum < n_lines && terminal[linenum][0] != '$') {
                        std::stringstream ss(terminal[linenum++]);
                        std::string descriptor;
                        std::string file_name;

                        ss >> descriptor >> file_name;

                        if (descriptor == "dir") {
                            cwd->add_dir(file_name);
                        } else {
                            size_t size = std::stoi(descriptor);
                            cwd->add_file(file_name, size);
                        }
                    }
                }
            }
        }
};

std::vector<std::string> get_inputs(std::ifstream &input) {
    std::vector<std::string> input_lines;
    std::string line;

    while (std::getline(input, line)) {
        input_lines.push_back(line);
    }

    return input_lines;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    std::ifstream input(argv[1]);
    if (!input.is_open()) {
        std::cerr << "Could not open input file " << argv[1] << std::endl;
        return 2;
    }

    auto inputs = get_inputs(input);
    TraverseDirectoryTree tree(inputs);

    auto sizes = tree.get_root()->directory_sizes();

    auto small_dirs = sizes 
                    | rv::filter([](auto& p) {
                                    auto &&[dir_name, size] = p;
                                    return size < 100000;
                                 });

    auto total_small_dirs = std::accumulate(small_dirs.begin(), small_dirs.end(), 0, [](auto acc, auto& p) {
        auto &&[dir_name, size] = p;
        return acc + size;
    });

    std::cout << "Part 1" << std::endl;
    std::cout << total_small_dirs << std::endl;

    std::cout << "Part 2" << std::endl;

    const size_t disk_space = 70000000;
    const size_t used = tree.get_root()->total_size();
    const size_t available = disk_space - used;

    const size_t needed = 30000000;

    if (available >= needed) {
        std::cout << "No additional space required" << std::endl;
        std::cout << 0 << std::endl;
        return 0;
    }

    const size_t must_delete = needed - available;

    std::sort(sizes.begin(), sizes.end(), [](auto& p1, auto& p2) {
                                             auto &&[dir_name1, size1] = p1;
                                             auto &&[dir_name2, size2] = p2;
                                             return size1 < size2;
                                           });
    
    auto big_enough = sizes 
                    | rv::filter([must_delete](auto& p) {
                                    auto &&[dir_name, size] = p;
                                    return size >= must_delete;
                                 })
                    | rv::take(1);

    for (auto& [dir_name, size] : big_enough) {
        std::cout << dir_name << " " << size << std::endl; 
    }

    return 0; 
}    