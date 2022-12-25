#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <limits>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

long snafu_to_decimal(const std::string &s) {
    long result = 0;
    for (auto c : s) {
        result *= 5;
        if (c >= '0' && c <= '2') {
            result += c - '0';
        } else if (c == '-') {
            result -= 1;
        } else if (c == '=') {
            result -=2;
        }
    }
    return result;
}

std::string decimal_to_snafu(long n) {
    std::string result;
    std::vector<int> quidgets;

    while (n > 0) {
        int quid = n % 5;
        if (quid > 2) {
            quid -= 5;
            n += 5;
        }
        quidgets.push_back(quid);
        n /= 5;
    }

    std::reverse(quidgets.begin(), quidgets.end());
    for (auto q : quidgets) {
        if (q >= 0 && q <= 2) {
            result += q + '0';
        } else if (q == -1) {
            result += '-';
        } else if (q == -2) {
            result += '=';
        }
    }

    return result;
}

std::vector<std::string> get_inputs(std::istream &input) {
    std::vector<std::string> lines;

    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty())
            lines.push_back(line);
    }

    return lines;
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

    std::cout << "Part 1" << std::endl;

    auto snafus = get_inputs(input);

    auto results = snafus | rv::transform(snafu_to_decimal);
    auto answer = std::accumulate(results.begin(), results.end(), 0LL);

    std::cout << answer << std::endl;
    std::cout << decimal_to_snafu(answer) << std::endl;
    std::cout << snafu_to_decimal(decimal_to_snafu(answer)) << std::endl;

    return 0;
}