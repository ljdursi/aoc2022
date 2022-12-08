#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

auto get_inputs(std::ifstream &input) {
    std::vector<std::vector<int>> row_inputs;
    std::vector<std::vector<int>> col_inputs;
    std::string line;

    while (std::getline(input, line)) {
        std::vector<int> input_row;
        int col = 0;
        for (const auto& c : line) {
            if (c > '9' || c < '0') {
                continue;
            }
            int val = (int)(c - '0');
            input_row.push_back(val);
            if (col_inputs.size() > col) {
                col_inputs[col].push_back(val);
            } else {
                col_inputs.push_back({val});
            }
            col++;
        }
        row_inputs.push_back(input_row);
    }

    return std::make_pair(row_inputs, col_inputs);
}

std::vector<bool> visibility_of_row(const std::vector<int> &row) {

    std::vector<bool> visible_from_left;
    std::vector<bool> visible_from_right_reversed;
    int max_so_far = -1;

    for (const auto height : row) {
        visible_from_left.push_back(height > max_so_far);
        max_so_far = std::max(max_so_far, height);
    }

    max_so_far = -1;
    for (const auto height : row | rv::reverse) {
        visible_from_right_reversed.push_back(height > max_so_far);
        max_so_far = std::max(max_so_far, height);
    }

    auto visible_from_right = visible_from_right_reversed | rv::reverse;

    auto visible = rv::zip(visible_from_left, visible_from_right) 
                 | rv::transform([](auto &&t) {
                                   auto &&[left, right] = t;
                                   return left || right;
                   }) 
                 | ranges::to<std::vector<bool>>;

    return visible;
}

std::vector<int> n_seen_in_row(const std::vector<int> &row) {
    const int n = row.size();
    std::vector<int> init_most_recent(10, -1);

    auto most_recent = init_most_recent;
    auto seen_to_left = [&most_recent](auto &&t) {
        auto &&[i, height] = t;
        int last = 0;
        for (int h=height; h<10; h++) {
            if (most_recent[h] == -1)
                continue;

            if (most_recent[h] > last)
                last = most_recent[h];
        } 
        most_recent[height] = i;
        return i - last;
    };

    auto n_seen_to_left = row 
                          | rv::enumerate 
                          | rv::transform(seen_to_left)
                          | ranges::to<std::vector<int>>;
    
    most_recent = init_most_recent;
    auto n_seen_to_right = rv::reverse(row)
                           | rv::enumerate
                           | rv::transform(seen_to_left)
                           | ranges::to<std::vector<int>>;

    auto n_seen_score = rv::zip(n_seen_to_left, rv::reverse(n_seen_to_right))
                      | rv::transform([](auto &&t) { auto &&[left, right] = t; return left * right; })
                      | ranges::to<std::vector<int>>;

    return n_seen_score ;
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

    auto &&[row_inputs, col_inputs] = get_inputs(input);
    const size_t n = row_inputs.size();
    const size_t m = col_inputs.size();

    auto row_visibility = row_inputs | rv::transform(visibility_of_row) | rv::enumerate;
    auto col_visibility = col_inputs | rv::transform(visibility_of_row) | rv::enumerate;

    auto visibilities = rv::cartesian_product(row_visibility, col_visibility)
                      | rv::transform([](auto &&t){
                                         auto &&[rv, cv] = t;
                                         auto &&[i, row] = rv;
                                         auto &&[j, col] = cv;
                                         return (row[j] || col[i]) ? 1 : 0;
                                      })
                      | ranges::to<std::vector<int>>;

    auto n_visible = ranges::accumulate(visibilities, 0, std::plus<int>());

    std::cout << "Part 1: " << std::endl;
    std::cout << n_visible << std::endl;

    auto row_seen_score = row_inputs | rv::transform(n_seen_in_row) | rv::enumerate;
    auto col_seen_score = col_inputs | rv::transform(n_seen_in_row) | rv::enumerate;

    auto seen_scores = rv::cartesian_product(row_seen_score, col_seen_score)
                     | rv::transform([](auto &&t){
                                         auto &&[rv, cv] = t;
                                         auto &&[i, row] = rv;
                                         auto &&[j, col] = cv;
                                         return (row[j] * col[i]);
                                      })
                      | ranges::to<std::vector<int>>;

    auto max_seen = ranges::accumulate(seen_scores, 0, ranges::max);
    std::cout << "Part 2: " << std::endl;
    std::cout << max_seen << std::endl;

    return 0;
}