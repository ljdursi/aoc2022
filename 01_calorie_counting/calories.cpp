#include <algorithm>
#include <numeric>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

std::vector<std::vector<int>> get_inputs(std::ifstream &input) {
    // read a list of list of integers from the input file,
    // with each list separated by a blank line

    std::vector<std::vector<int>> inputs;
    std::vector<int> current_input;
    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            inputs.push_back(current_input);
            current_input.clear();
        } else {
            current_input.push_back(std::stoi(line));
        }    
    }

    if (!current_input.empty()) {
        inputs.push_back(current_input);
    }

    return inputs;
}

void print_inputs(std::vector<std::vector<int>> inputs) {
    // print the list of list of integers

    for (auto input : inputs) {
        std::cout << "(" << input.size() << ") ";
        for (auto n : input) {
            std::cout << n << " ";
        }
        std::cout << std::endl;
    }
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

    std::vector<std::vector<int>> calories = get_inputs(input);

    std::cout << "Part 1" << std::endl;

    // sum each vector
    std::vector<int> total_calories;
    std::transform(calories.begin(), calories.end(), std::back_inserter(total_calories), [](std::vector<int> &input) {
        return std::accumulate(input.begin(), input.end(), 0);
    });

    // find the max
    int max_cals = *std::max_element(total_calories.begin(), total_calories.end());
    std::cout << "Max calories: " << std::endl << max_cals << std::endl;

    std::cout << "Part 2" << std::endl;

    // find the sum of the max three values
    std::sort(total_calories.begin(), total_calories.end(), std::greater<>());
    int max_three_sum = std::accumulate(total_calories.begin(), total_calories.begin() + 3, 0);

    std::cout << max_three_sum << std::endl;

    return 0;
}