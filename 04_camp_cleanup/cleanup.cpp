#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "range/v3/all.hpp"
#include <algorithm>
#include <regex>


class CleanupRange {
    // inclusive range of cleanup areas
    int first;
    int last;

    // implementation of the range interface
    class Iterator {
        int current;
    public:
        Iterator(int first) : current(first) {}
        int operator*() const { return current; }
        Iterator& operator++() { ++current; return *this; }
        bool operator!=(const Iterator& other) const { return current != other.current; }
    };

    class Sentinel {
        int last;
    public:
        Sentinel(int last) : last(last) {}
        bool operator!=(const Iterator& other) const { return other != last; }
    };

public:
    Iterator begin() const { return Iterator(first); }
    Sentinel end() const { return Sentinel(last + 1); }

    friend std::ostream& operator<<(std::ostream &os, const CleanupRange &range) {
        os << "(" << range.first << ", " << range.last << ")";
        return os;
    };

    CleanupRange(int first, int last) : first(first), last(last) {}
    CleanupRange(std::pair<int, int> firstlast) : first(firstlast.first), last(firstlast.second) {}

    bool contains(int value) const {
        return value >= first && value <= last;
    }

    bool contains(const CleanupRange &other) const {
        return contains(other.first) && contains(other.last);
    }

    bool contained_by(const CleanupRange &other) const {
        return other.contains(first) && other.contains(last);
    }

    bool overlaps(const CleanupRange &other) const {
        return contains(other.first) || contains(other.last) || other.contains(first) || other.contains(last);
    }
};

namespace rv = ranges::views;

std::vector<std::pair<CleanupRange, CleanupRange>> get_inputs(std::ifstream &input) {
    std::vector<std::string> input_lines(std::istream_iterator<std::string>(input), {});

    auto results = input_lines 
                    | rv::transform([](const std::string &line) {
                        std::regex regex{R"([\s,-]+)"};
                        std::sregex_token_iterator it{line.begin(), line.end(), regex, -1};
                        std::vector<std::string> items{it, {}};
                        return items;
                       })
                    | rv::transform([](auto &&stringvec) {
                        auto first = std::make_pair(std::stoi(stringvec[0]), std::stoi(stringvec[1]));
                        auto second = std::make_pair(std::stoi(stringvec[2]), std::stoi(stringvec[3]));
                        return std::make_pair(first, second);
                      }) 
                    | rv::transform([](auto &&pairs) {
                        auto first = pairs.first;
                        auto second = pairs.second;
                        return std::make_pair(CleanupRange(first), CleanupRange(second));
                      });

    std::vector<std::pair<CleanupRange, CleanupRange>> output;
    ranges::copy(results, std::back_inserter(output));
    return output;
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
    int n_fully_contained = std::transform_reduce(inputs.begin(), inputs.end(), 
                                                  0, std::plus<int>(),
                                                  [](const auto &pair) {
                                                    auto range1= pair.first;
                                                    auto range2 = pair.second;
                                                    return (range1.contains(range2) || range2.contains(range1)) ? 1 : 0;
                                                  });

    std::cout << "Part 1" << std::endl;
    std::cout << n_fully_contained << std::endl;

    int n_overlaps = std::transform_reduce(inputs.begin(), inputs.end(), 
                                           0, std::plus<int>(),
                                           [](const auto &pair) {
                                              auto range1= pair.first;
                                              auto range2 = pair.second;
                                              return range1.overlaps(range2) ? 1 : 0;
                                            });

    std::cout << "Part 2" << std::endl;
    std::cout << n_overlaps << std::endl;

    return 0;
}