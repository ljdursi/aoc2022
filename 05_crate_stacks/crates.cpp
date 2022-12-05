#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <deque>
#include <numeric>
#include <cassert>
#include <string>
#include "range/v3/all.hpp"

namespace rv = ranges::views;
using move = std::tuple<int, int, int>;

class CrateStacks {
private:
    int n_stacks;
    std::vector<std::deque<char>> stacks;

public:
    CrateStacks(std::vector<std::vector<char>> inputs) : n_stacks(inputs.size()) {
        stacks = inputs | rv::transform([](auto &&vec) {
                                           std::deque<char> stack;
                                           ranges::copy(vec, std::front_inserter(stack));
                                           return stack;
                                        })
                        | ranges::to<std::vector<std::deque<char>>>;
    }

    int get_n_stacks() const {
        return n_stacks;
    }

    friend std::ostream& operator<< (std::ostream &os, const CrateStacks &cratestacks) {
        const size_t maxheight = std::accumulate(cratestacks.stacks.begin(), cratestacks.stacks.end(),
                                                 (size_t) 0,
                                                 [](size_t curmax, auto &stack){return std::max(curmax, stack.size());});

        std::vector<std::string> strings = rv::iota((size_t)0, maxheight) 
                                         | rv::reverse 
                                         | rv::transform([&cratestacks](size_t height) {
                                            std::string result = cratestacks.stacks 
                                                        | rv::transform([&height](const auto &stack) {
                                                                                  std::string empty = "   ";
                                                                                  if (stack.size() > height)
                                                                                    return std::string("[") + stack[height] + std::string("]");
                                                                                  else
                                                                                    return empty;
                                                          })
                                                        | rv::join(" ")
                                                        | ranges::to<std::string>;
                                            return result;
                                           })
                                          | ranges::to<std::vector<std::string>>;

        for (const auto &s : strings)
            os << s << std::endl;

        auto labels = rv::iota(1, cratestacks.n_stacks+1) 
                    | rv::transform([](int i) { return " " + std::to_string(i) + " "; }) 
                    | rv::join(" ") 
                    | ranges::to<std::string>;

        os << labels << std::endl;

        return os;
    }

    void move(const int n, const int from, const int to, bool all_at_once = false) {

        assert(from > 0 && from <= n_stacks);
        assert(to > 0 && to <= n_stacks);

        const int zero_indexed_from = from-1;
        const int zero_indexed_to = to-1;
        assert(n <= stacks[zero_indexed_from].size());

        if (!all_at_once) {
            for (int i = 0; i < n; ++i) {
                stacks[zero_indexed_to].push_back(stacks[zero_indexed_from].back());
                stacks[zero_indexed_from].pop_back();
            }
        } else {
            std::deque<char> temp;
            for (int i = 0; i < n; ++i) {
                temp.push_back(stacks[zero_indexed_from].back());
                stacks[zero_indexed_from].pop_back();
            }
            for (int i = 0; i < n; ++i) {
                stacks[zero_indexed_to].push_back(temp.back());
                temp.pop_back();
            }
        }
    }

    std::string top() const {
        return stacks | rv::transform([](const auto &stack) { return stack.back(); }) 
                      | ranges::to<std::string>;
    }
};


std::pair<CrateStacks, std::vector<move>> get_inputs(std::ifstream &input) {
    std::vector<std::string> input_lines;
    std::string line;

    while (std::getline(input, line)) {
        input_lines.push_back(line);
    }

    auto items = input_lines  
                 | rv::take_while([](const std::string &s) { return !s.empty(); })
                 | rv::transform([](const std::string &s) { return " " + s; })
                 | rv::transform([](const std::string &line) {
                                    std::vector<std::pair<int, std::string>> items;
                                    for (int i=0; i<line.length(); i+=4) {
                                        items.push_back(std::make_pair(i/4, line.substr(i, 4)));
                                    }
                                    return items;
                                 })
                 | rv::filter([](const auto &v) { return !v.empty(); })
                 | rv::join
                 | rv::filter([](const auto &p) { return std::get<1>(p) != "    "; })
                 | rv::filter([](const auto &p) { return std::get<1>(p)[1] == '['; })
                 | rv::transform([](const auto &p) { return std::make_pair(std::get<0>(p), std::get<1>(p)[2]); })
                 | ranges::to<std::vector<std::pair<int, char>>>;

    int n_stacks = std::accumulate(items.begin(), items.end(), 0, [](int curmax, const auto &p) {
        return std::max(curmax, std::get<0>(p) + 1);
    });

    std::vector<std::vector<char>> inputs(n_stacks);
    for (const auto &p : items ) {
        int index = std::get<0>(p);
        char c = std::get<1>(p);
        inputs[index].push_back(c);
    }

    CrateStacks cratestacks(inputs);

    auto moves = input_lines 
               | rv::drop_while([](const auto &s) { return !s.empty(); })
               | rv::drop(1)
               | rv::transform([](const auto &line) {
                                    std::regex regex{R"(\d+)"};
                                    std::sregex_iterator it(line.begin(), line.end(), regex);
                                    std::vector<std::string> result;
                                    for (auto &i = it; i != std::sregex_iterator(); ++i) {
                                        result.push_back(i->str());
                                    }
                                    return std::make_tuple(std::stoi(result[0]), std::stoi(result[1]), std::stoi(result[2]));
                               })
               | ranges::to<std::vector<std::tuple<int, int, int>>>;

    return std::make_pair(cratestacks, moves);
};

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
    auto p1stacks = inputs.first;
    auto p2stacks = inputs.first;
    auto moves = inputs.second;

    for (const auto move: moves) {
        p1stacks.move(std::get<0>(move), std::get<1>(move), std::get<2>(move));
    }

    std::cout << "Part 1" << std::endl;
    std::cout << p1stacks.top() << std::endl;

    for (const auto move: moves) {
        p2stacks.move(std::get<0>(move), std::get<1>(move), std::get<2>(move), true);
    }

    std::cout << "Part 2" << std::endl;
    std::cout << p2stacks.top() << std::endl;

    return 0;
}