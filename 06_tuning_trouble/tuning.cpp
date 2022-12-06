#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <algorithm>
#include <iterator>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

int marker_start(std::string datapacket, int n) {

    auto process = datapacket 
                 | rv::sliding(n)
                 | rv::enumerate
                 | rv::transform([](auto &&t) {
                                    auto &&[i, window] = t;
                                    auto strset = std::set<char>(window.begin(), window.end());
                                    bool all_unique = strset.size() == window.size();
                                    std::string window_str(window.begin(), window.end());
                                    return std::make_tuple(i, all_unique, window_str);
                                 })
                 | ranges::to<std::vector<std::tuple<size_t, bool, std::string>>>;

    // TODO: can't figure out how to this lazily with ranges v3 + MacOS clang

    auto results = std::find_if(std::begin(process), std::end(process),
                                [](auto &&t) {
                                   auto &&[i, all_unique, window] = t;
                                   return all_unique;
                                 });

    auto &&[i, all_unique, window] = *results;
    return i + n;
}

std::string get_inputs(std::ifstream &input) {
    std::vector<std::string> input_lines;
    std::string line;

    while (std::getline(input, line)) {
        input_lines.push_back(line);
    }

    return input_lines[0];
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

    std::cout << "Part 1" << std::endl;
    std::cout << marker_start(inputs, 4) << std::endl;

    std::cout << "Part 2" << std::endl;
    std::cout << marker_start(inputs, 14) << std::endl;
}    