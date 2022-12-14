#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <regex>
#include <variant>
#include <compare>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

std::vector<std::string> tokenize(const std::string& s) {
    std::vector<std::string> tokens;
    std::regex re(R"(\[|\]|\d+)");

    auto it = std::sregex_iterator(s.begin(), s.end(), re);
    auto end = std::sregex_iterator();

    while (it != end) {
        tokens.push_back(it->str());
        ++it;
    }
    return tokens;
}

struct Packet : public std::variant<int, std::vector<Packet>> {};

std::ostream& operator<<(std::ostream& os, const Packet& packet) {
    if (std::holds_alternative<int>(packet)) {
        return os << std::get<int>(packet);
    } else {
        os << "[";
        for (const auto& p : std::get<std::vector<Packet>>(packet)) {
            os << p << " ";
        }
        os << "]";
    }
    return os;
}


std::vector<std::string> subtokens(const std::vector<std::string>& tokens, const int idx) {
    // find the ']' that matches the '[' at idx
    int num_open = 1;
    int i = idx+1;
    while (num_open > 0) {
        if (tokens[i] == "[") {
            ++num_open;
        } else if (tokens[i] == "]") {
            --num_open;
        }
        ++i;
    }

    return std::vector<std::string>(tokens.begin()+idx, tokens.begin()+i);
}

Packet from_tokens(const std::vector<std::string>& tokens, const int start=1) {
    if (start == tokens.size()-1 && tokens[start] != "]" && tokens[start] != "[") {
        return Packet{std::stoi(tokens[start])};
    }

    std::vector<Packet> result;
    for (int i=start; i<tokens.size(); ++i) {
        if (tokens[i] == "[") {
            auto sub = subtokens(tokens, i);
            result.push_back(from_tokens(sub));
            i += sub.size()-1;
        } else if (tokens[i] == "]") {
            break;
        } else {
            result.push_back(Packet{std::stoi(tokens[i])});
        }
    }
    return Packet{result};
};

Packet vector_packet_from_int(int i) {
    std::vector<Packet> vp = {Packet{i}};
    return Packet{vp};
}

auto operator<=>(const Packet &left, const Packet &right) {
    if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
        return std::get<int>(left) <=> std::get<int>(right);
    } else if (std::holds_alternative<int>(left) && std::holds_alternative<std::vector<Packet>>(right)) {
        return vector_packet_from_int(std::get<int>(left)) <=> right;
    } else if (std::holds_alternative<std::vector<Packet>>(left) && std::holds_alternative<int>(right)) {
        return left <=> vector_packet_from_int(std::get<int>(right));
    } else {
        auto left_vec = std::get<std::vector<Packet>>(left);
        auto right_vec = std::get<std::vector<Packet>>(right);

        for (int i=0; i<std::min(left_vec.size(), right_vec.size()); ++i) {
            auto cmp = left_vec[i] <=> right_vec[i];
            if (!(cmp == 0)) {
                return left_vec[i] <=> right_vec[i];
            }
        }
        return left_vec.size() <=> right_vec.size();
    }
}

std::vector<std::string> get_inputs(std::istream& input) {
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty())
            lines.push_back(line);
    }
    return lines;
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

    auto lines = get_inputs(input);

    auto tokens = lines 
                | rv::transform(tokenize) 
                | ranges::to<std::vector<std::vector<std::string>>>();

    auto packets = lines 
                | rv::transform(tokenize) 
                | rv::transform([](const std::vector<std::string> &v){return from_tokens(v);})
                | ranges::to<std::vector<Packet>>();

    auto in_right_order = packets
                        | rv::chunk(2)
                        | rv::transform([](const auto& v) { 
                                           auto left = v[0];
                                           auto right = v[1];
                                           return left <= right; })
                        | ranges::to<std::vector<bool>>();

    auto scores = in_right_order 
                | rv::enumerate
                | rv::filter([](const auto& p) { return p.second; }) 
                | rv::transform([](const auto& p) { return p.first + 1; }) 
                | ranges::to<std::vector<int>>();

    std::cout << "Part 1" << std::endl;
    std::cout << std::accumulate(scores.begin(), scores.end(), 0) << std::endl;

    std::cout << "Part 2" << std::endl;

    Packet decoder1 = from_tokens(tokenize("[[2]]"));
    packets.push_back(decoder1);
    Packet decoder2 = from_tokens(tokenize("[[6]]"));
    packets.push_back(decoder2);

    std::sort(packets.begin(), packets.end(), [](const Packet& left, const Packet& right) { return left < right; });

    auto indicies = packets 
                | rv::enumerate
                | rv::filter([&](const auto& p) { return p.second == decoder1 || p.second == decoder2; })
                | rv::transform([](const auto& p) { return p.first+1; })
                | ranges::to<std::vector<int>>();

    std::cout << std::accumulate(indicies.begin(), indicies.end(), 1, std::multiplies<int>()) << std::endl;
    return 0;
}