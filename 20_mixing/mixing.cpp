#include <vector>
#include <list>
#include <cassert>
#include <iostream>
#include <fstream>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

struct message {
    std::vector<std::pair<long, size_t>> items_and_cur_indices;
    std::vector<std::pair<long, size_t>> shuffled_items_and_prev_indices;
    size_t n;
    int head = 0;

    message() : n(0) {}
    message(std::vector<long> items) : n(items.size()) {
        for (int i=0; i < n; ++i) {
            items_and_cur_indices.push_back({items[i], i});
        }
        shuffled_items_and_prev_indices = items_and_cur_indices;
    }

    size_t index(int i) const {
        while (i < 0) {
            i += n;
        }
        while (i >= n) {
            i -= n;
        }
        return i;
    }

    size_t location_of_zero() {
        auto it = std::find_if(shuffled_items_and_prev_indices.begin(), shuffled_items_and_prev_indices.end(), [](auto p) { return p.first == 0; });
        return index(it - shuffled_items_and_prev_indices.begin() - head);
    }

    long operator[](size_t i) const {
        return shuffled_items_and_prev_indices.at(index(i + head)).first;
    }

    size_t bubble(size_t cur_idx, int direction) {
        size_t other_idx = index(cur_idx + direction);

        const auto orig_idx1 = shuffled_items_and_prev_indices.at(cur_idx).second;
        const auto orig_idx2 = shuffled_items_and_prev_indices.at(other_idx).second;

        std::swap(shuffled_items_and_prev_indices[cur_idx], shuffled_items_and_prev_indices[other_idx]);
        std::swap(items_and_cur_indices[orig_idx1].second, items_and_cur_indices[orig_idx2].second);

        if (other_idx == head)
            head = cur_idx;

        return other_idx;
    }

    void move_ith(size_t i) {
        auto &[value, shuffled_loc] = items_and_cur_indices.at(i);

        int dir = (value < 0) ? -1 : +1;
        int nsteps = std::abs(value) % (n - 1);
        for (int step=0; step<nsteps; step++) {
            shuffled_loc = bubble(shuffled_loc, dir);
        }
    }   

    void validate() {
        for (size_t i = 0; i < n; ++i) {
            auto &[value, loc] = items_and_cur_indices[i];
            assert(value == shuffled_items_and_prev_indices[loc].first);
            assert(i == shuffled_items_and_prev_indices[loc].second);
        }
        for (size_t i = 0; i < n; ++i) {
            auto &[value, loc] = shuffled_items_and_prev_indices[i];
            assert(value == items_and_cur_indices[loc].first);
            assert(i == items_and_cur_indices[loc].second);
        }
    }

    std::string to_string() const {
        std::string result;
        for (size_t i = 0; i < n; ++i) {
            result += std::to_string(items_and_cur_indices[i].first) + " ";
        }
        result += "\n";
        for (size_t i = head; i < n; ++i) {
            result += std::to_string(shuffled_items_and_prev_indices[i].first) + " ";
        }
        for (size_t i = 0; i < head; ++i) {
            result += std::to_string(shuffled_items_and_prev_indices[i].first) + " ";
        }
        return result + "\n";
    }
};

std::vector<long> get_inputs(std::ifstream &input) {
    std::vector<long> result;
    long i;
    while (input >> i) {
        result.push_back(i);
    }
    return result;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    std::ifstream input(argv[1]);
    if (!input.is_open()) {
        std::cerr << "Could not open input file " << argv[1] << std::endl;
        return 2;
    }

    const auto nums = get_inputs(input);

    message m(nums);
    size_t n = m.n;

    for (size_t i = 0; i < n; ++i) {
        m.move_ith(i);
    }

    size_t loc = m.location_of_zero();
    std::cout << "0 is at " << loc << std::endl;

    long sum = 0;
    for (size_t offset : std::vector<size_t>{1000, 2000, 3000}) {
        sum += m[offset + loc];
    }
    std::cout << "Part 1" << std::endl;
    std::cout << sum << std::endl;

    auto long_nums = nums | rv::transform([](const int n) { return n * 811589153L; }) | ranges::to<std::vector<long>>();
    auto m2 = message(long_nums);
    for (int round=0; round < 10; round++) {
        for (size_t i = 0; i < n; ++i) {
            m2.move_ith(i);
        }
    }
    
    sum = 0;
    loc = m2.location_of_zero();
    for (size_t offset : std::vector<size_t>{1000, 2000, 3000}) {
        sum += m2[offset + loc];
    }

    std::cout << "Part 2" << std::endl;
    std::cout << sum << std::endl;

    return 0;
}