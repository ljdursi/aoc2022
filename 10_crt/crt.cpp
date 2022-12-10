#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

enum struct opcode {NOP, ADDX};

class Instruction {
    opcode op;

public:
    int arg;
    size_t cycles;
    Instruction(opcode op, int arg=0) : op(op), arg(arg) {
        switch (op) {
            case opcode::NOP: cycles = 1; break;
            case opcode::ADDX: cycles = 2; break;
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const Instruction& i) {
        switch (i.op) {
            case opcode::NOP: os << "NOP"; break;
            case opcode::ADDX: os << "ADDX " << i.arg; break;
        }
        return os;
    }
};

std::vector<Instruction> get_inputs(std::ifstream &input) {
    std::vector<std::string> input_lines;
    std::string line;

    while (std::getline(input, line)) {
        input_lines.push_back(line);
    }

    auto items = input_lines  
                 | rv::transform([](const std::string &line) {
                                    std::stringstream ss(line);
                                    std::string item;
                                    std::vector<std::string> items;
                                    while (std::getline(ss, item, ' ')) {
                                        items.push_back(item);
                                    }
                                    return items;
                                 })
                 | rv::transform([](const auto &v) {
                                    if (v[0] == "addx") 
                                        return Instruction(opcode::ADDX, std::stoi(v[1]));
                                    else 
                                        return Instruction(opcode::NOP);
                                 })
                 | ranges::to<std::vector<Instruction>>;

    return items;
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
    inputs.insert(inputs.begin(), Instruction(opcode::NOP));

    // we really some range fold operators
    std::pair<size_t, int> init_state = {0, 1};
    auto state = init_state;

    auto timeline = inputs
                  | rv::transform([&state](const Instruction &i) { 
                                    std::vector<std::pair<size_t, int>> result;
                                    for (int j = 0; j < i.cycles-1; j++) {
                                        state.first++;
                                        result.push_back(state);
                                    }
                                    state.first++;
                                    state.second += i.arg; 
                                    result.push_back(state);
                                    return result;
                                  })
                  | rv::join
                  | ranges::to<std::vector<std::pair<size_t, int>>>;

    auto signal = timeline
                  | rv::drop(19)
                  | rv::stride(40)
                  | rv::transform([](const auto &p) { auto &&[t, x] = p; return t*x; })
                  | ranges::to<std::vector<int>>;

    auto result = std::accumulate(signal.begin(), signal.end(), 0);
    std::cout << "Part 1" << std::endl;
    std::cout << result << std::endl;

    auto pixels = timeline
                  | rv::take(240)
                  | rv::transform([](auto &p) { 
                                    auto &&[time, sprite_position] = p;
                                    int pixel_drawn = (int)(time-1) % 40;

                                    if (pixel_drawn < sprite_position-1 || pixel_drawn > sprite_position+1) {
                                        return '.';
                                    } else {
                                        return '#';
                                    }
                                  })
                  | rv::chunk(40)
                  | rv::transform([](const auto &v) { 
                                    std::string s(v.begin(), v.end());
                                    return s;
                                  })
                  | ranges::to<std::vector<std::string>>;

    std::cout << "Part 1" << std::endl;
    std::cout << std::endl;
    for (auto &&p : pixels) {
        std::cout << p << std::endl;
    }

    return 0;
}
  