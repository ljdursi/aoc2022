#include <vector>
#include <deque>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include <algorithm>
#include <cassert>
#include <limits>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

class Monkey {
public:
    std::deque<long> current_items;
    std::function<long(long)> worry_update;
    std::function<long(long)> throw_to;
    long ninspections = 0;
    long div_product = 0;
    bool worrying = false;

    Monkey(std::vector<long> items,
           std::function<long(long)> update,
           std::function<long(long)> throw_to,
           bool worrying = false,
           long div_product = LONG_MAX) : worry_update(update),
                                          throw_to(throw_to),
                                          ninspections(0),
                                          worrying(worrying),
                                          div_product(div_product) {
            this->current_items = std::deque<long>(items.begin(), items.end());
    }

    std::vector<std::pair<long, long>> throw_items() {
        if (current_items.empty()) return {};

        std::vector<std::pair<long, long>> thrown_items;
        while (!current_items.empty()) {
            auto item = current_items.front();
            auto recipient = throw_to(item);

            thrown_items.push_back(std::make_pair(item, recipient));
            current_items.pop_front();
        }

        return thrown_items;
    }

    void catch_item(long item) {
        current_items.push_back(item);
    }

    void inspect_items() {
        const long relax_factor = (worrying ? 1 : 3);
        const long range = div_product;

        current_items = current_items
                              | rv::transform(worry_update)
                              | rv::transform([relax_factor](long i) { return i / relax_factor; })
                              | rv::transform([range](long i) { return i % range; })
                              | ranges::to<std::deque<long>>;
        ninspections += current_items.size();
    }
};

class BarrelOfMonkeys {
    std::vector<Monkey> monkeys;

    public:
        BarrelOfMonkeys(std::vector<Monkey> monkeys) : monkeys(monkeys) {}

        void make_worrying() {
            for (auto &monkey : monkeys) {
                monkey.worrying = true;
            }
        }

        void round() {
            // update the amount of worry for the items each monkey has
            for (auto &monkey : monkeys)  {
                monkey.inspect_items();
                for (const auto &thrown_item : monkey.throw_items()) {;
                    const auto &[item, recipient] = thrown_item;

                    assert(recipient < monkeys.size());
                    monkeys[recipient].catch_item(item);
                }
            }
        }

        friend std::ostream& operator<<(std::ostream& os, const BarrelOfMonkeys& b) {
            long i = 0;
            for (auto &monkey : b.monkeys) {
                os << "Monkey " << i++ << ": ";
                if (monkey.current_items.empty()) {
                    os << std::endl;
                    continue;
                }
                auto str = std::transform_reduce(monkey.current_items.begin(),
                                                 monkey.current_items.end(),
                                                 std::string(),
                                                 [](std::string a, std::string b) { return a + ", " + b; },
                                                 [](long i) { return std::to_string(i); });
                os << str.substr(2) << std::endl;
            }
            return os;
        }

        std::vector<long> inspected() {
            return monkeys | rv::transform([](const Monkey &m) { return m.ninspections; }) | ranges::to<std::vector<long>>;
        }
};

std::vector<long> longegers_from_line(std::string line) {
    std::regex r("(\\d+)");
    std::sregex_iterator sm(line.begin(), line.end(), r);
    std::sregex_iterator end;

    std::vector<long> items;

    while (sm != end) {
        std::smatch match = *sm;
        items.push_back(std::stoi(match[1]));
        sm++;
    }
    return items;
}

std::function<long(long)> update_fn(const std::string &line) {
    auto multiply_by = [](long x) -> std::function<long(long)> { return [x](long y) { return (long)(x * y); }; };
    auto add_to      = [](long x) -> std::function<long(long)> { return [x](long y) { return (long)(x + y); }; };
    auto square      = []() -> std::function<long(long)> { return [](long y) { return (long)(y * y); }; };

    std::regex add_re("  Operation: new = old \\+ (\\d+)");
    std::regex mul_re("  Operation: new = old \\* (\\d+)");
    std::regex sqr_re("  Operation: new = old \\* old");

    std::smatch match;
    if (std::regex_match(line, match, add_re)) {
        long operand = std::stoi(match[1]);
        return add_to(operand);
    } else if (std::regex_match(line, match, mul_re)) {
        long operand = std::stoi(match[1]);
        return multiply_by(operand);
    } else
        return square();
}

std::function<long(long)> pass_fn(std::string testline, std::string trueline, std::string falseline) {
    auto pass_to = [](long divisand, long m1, long m2) {
        return [divisand, m1, m2](long item) {
            if (item % divisand == 0) 
                return m1;
            else 
                return m2;
         };
    };

    std::regex test_re ("  Test: divisible by (\\d+)");
    std::regex true_re ("    If true: throw to monkey (\\d+)");
    std::regex false_re("    If false: throw to monkey (\\d+)");
    std::smatch match;

    long divisand=-1, true_m=-1, false_m=-1;
    if (std::regex_match(testline, match, test_re))
        divisand = std::stoi(match[1]);
    if (std::regex_match(trueline, match, true_re))
        true_m = std::stoi(match[1]);
    if (std::regex_match(falseline, match, false_re))
        false_m = std::stoi(match[1]);

    return pass_to(divisand, true_m, false_m);
};

auto get_inputs(std::ifstream &input, bool worrying=false) {
    std::vector<std::string> input_lines;
    std::string line;

    while (std::getline(input, line)) {
        input_lines.push_back(line);
    }

    // extract the items
    const size_t lines_per_monkey = 7;

    auto items = input_lines  
                 | rv::chunk(lines_per_monkey)
                 | rv::transform([](const auto &v) { return longegers_from_line(v[1]); })
                 | ranges::to<std::vector<std::vector<long>>>();

    auto update_fns = input_lines
                 | rv::chunk(lines_per_monkey)
                 | rv::transform([](const auto &v) { return update_fn(v[2]); })
                 | ranges::to<std::vector<std::function<long(long)>>>();

    auto pass_fns = input_lines
                  | rv::chunk(lines_per_monkey)
                  | rv::transform([](const auto &v) {return pass_fn(v[3], v[4], v[5]);})
                  | ranges::to<std::vector<std::function<long(long)>>>();

    auto divisors = input_lines 
                  | rv::filter([](const auto &s) { return s.find("divisible by") != std::string::npos; })
                  | rv::transform([](const auto &s) {
                        std::regex re("[ A-Za-z:]*(\\d+)");
                        std::smatch match;
                        if (std::regex_match(s, match, re))
                            return std::stoi(match[1]);
                        else
                            return -1;
                    })
                  | ranges::to<std::vector<long>>();

    auto divisor_product = std::accumulate(divisors.begin(), divisors.end(), 1L, std::multiplies<long>());

    auto monkeys = rv::zip(items, update_fns, pass_fns)
                 | rv::transform([worrying, divisor_product](const auto &v) {
                        const auto &[items, update, pass] = v;
                        return Monkey(items, update, pass, worrying, divisor_product);
                    })
                 | ranges::to<std::vector<Monkey>>();

    return monkeys;
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

    std::cout << "Part 1" << std::endl;

    auto original_monkeys = get_inputs(input);
    auto unworrying_monkeys = original_monkeys;
    BarrelOfMonkeys barrel(unworrying_monkeys);

    for (int round = 0; round < 20; round++) {
        barrel.round();
    }
    auto inspected = barrel.inspected();
    auto scores = ranges::actions::sort(inspected) | rv::reverse | rv::take(2);
    auto total_score = std::accumulate(scores.begin(), scores.end(), 1L, std::multiplies<long>());

    std::cout << "Score: " << total_score << std::endl;

    std::cout << "Part 2" << std::endl;
    BarrelOfMonkeys barrel_of_worry(original_monkeys);
    barrel_of_worry.make_worrying();

    for (int round = 0; round < 10000; round++) {
        barrel_of_worry.round();
    }
 
    auto worryingly_inspected = barrel_of_worry.inspected();
    scores = ranges::actions::sort(worryingly_inspected) | rv::reverse | rv::take(2);
    total_score = std::accumulate(scores.begin(), scores.end(), 1L, std::multiplies<long>());
    std::cout << "Score: " << total_score << std::endl;

    return 0;
}