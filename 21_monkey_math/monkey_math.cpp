#include <map>
#include <tuple>
#include <variant>
#include <string>
#include <vector>
#include <type_traits>
#include <fstream>
#include <iostream>
#include <limits>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

enum struct op : size_t { add=0, sub=1, mul=2, div=3, eql=4, var=5, last };

std::map<std::string, op> valid_ops = {
    {"+", op::add},
    {"-", op::sub},
    {"*", op::mul},
    {"/", op::div},
    {"X", op::var},
    {"=", op::eql},
};

std::map<op, std::string> valid_ops_reverse = {
    {op::add, "+"},
    {op::sub, "-"},
    {op::mul, "*"},
    {op::div, "/"},
    {op::var, "X"},
    {op::eql, "="},
};

std::map<op, op> inverse = {
    {op::add, op::sub},
    {op::sub, op::add},
    {op::mul, op::div},
    {op::div, op::mul},
};

long calculate(op opr, long l, long r) {
    switch (opr) {
        case op::add: return l + r;
        case op::sub: return l - r;
        case op::mul: return l * r;
        case op::div: return l / r;
        default: return 0L;
    }
}

struct monkey_string_op : public std::variant<long, std::tuple<op, std::string, std::string>> {};

struct monkey_expr : public std::variant<long, op, std::vector<monkey_expr>> {};

monkey_expr build_expression(std::map<std::string, monkey_string_op> monkeys, std::string root) {
    if (std::holds_alternative<long>(monkeys.at(root))) {
        return monkey_expr{std::get<long>(monkeys.at(root))};
    }

    auto [opr, lhs, rhs] = std::get<std::tuple<op, std::string, std::string>>(monkeys.at(root));

    if (opr == op::var) {
        return monkey_expr{opr};
    }

    auto l_expr = build_expression(monkeys, lhs);
    auto r_expr = build_expression(monkeys, rhs);

    if (std::holds_alternative<long>(l_expr) && std::holds_alternative<long>(r_expr)) {
        return monkey_expr{calculate(opr, std::get<long>(l_expr), std::get<long>(r_expr))};
    }

    return monkey_expr{std::vector<monkey_expr>{monkey_expr{opr}, l_expr, r_expr}};
}

std::ostream& operator<<(std::ostream& os, const monkey_expr& expr) {
    if (std::holds_alternative<long>(expr)) {
        return os << std::get<long>(expr);
    } else if (std::holds_alternative<op>(expr)) {
        return os << valid_ops_reverse.at(std::get<op>(expr));
    } else {
        auto v = std::get<std::vector<monkey_expr>>(expr);
        op opr = std::get<op>(v[0]);
        monkey_expr lhs = v[1];
        monkey_expr rhs = v[2];

        os << "(";
        os << lhs;
        os << " " << valid_ops_reverse.at(opr) << " ";
        os << rhs;
        os << ")";
    }
    return os;
}

long evaluate(const std::map<std::string, monkey_string_op> &monkeys, const monkey_string_op &ms) {
    if (std::holds_alternative<long>(ms)) {
        return std::get<long>(ms);
    }

    const auto &[opr, lhs, rhs] = std::get<std::tuple<op, std::string, std::string>>(ms);

    long l = evaluate(monkeys, monkeys.at(lhs));
    long r = evaluate(monkeys, monkeys.at(rhs));

    return calculate(opr, l, r);
}

long evaluate(const std::map<std::string, monkey_string_op> &monkeys, const std::string &monkey) {
    return evaluate(monkeys, monkeys.at(monkey));
}

monkey_string_op parse_op(const std::string& s) {
    std::regex expression(R"((\w+) ([+-/*]) (\w+))");
    std::regex literal(R"((\d+))");

    monkey_string_op result;

    std::smatch match;
    if (std::regex_match(s, match, expression)) {
        result = monkey_string_op{std::make_tuple(valid_ops[match[2]], match[1], match[3])};
    } else if (std::regex_match(s, match, literal)) {
        result = monkey_string_op{std::stoi(match[1])};
    } else {
        throw std::runtime_error("Invalid input");
    }

    return result;
}

std::map<std::string, monkey_string_op> get_inputs(std::istream &input) {
    std::vector<std::string> lines;

    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) 
            lines.push_back(line);
    }

    auto result = lines | rv::transform([](const std::string& line) 
                                    {
                                        auto it = line.find(":");
                                        return std::make_pair(line.substr(0, it), parse_op(line.substr(it + 2))); 
                                    })
                        | ranges::to<std::map<std::string, monkey_string_op>>();

    return result;
}

long solve_for_x(monkey_expr l_expr, long rhs) {
    if (std::holds_alternative<long>(l_expr)) {
        return std::get<long>(l_expr);
    }

    std::vector<monkey_expr> current{monkey_expr{op::eql}, l_expr, monkey_expr{rhs}};
    auto v = std::get<std::vector<monkey_expr>>(l_expr);

    auto opr = std::get<op>(v[0]);
    auto ll = v[1];
    auto rr = v[2];
        
    if (std::holds_alternative<long>(rr)) {
        rhs = calculate(inverse[opr], rhs, std::get<long>(rr));
        l_expr = ll;
    } else {
        switch (opr) {
            case op::add:
                rhs = calculate(op::sub, rhs, std::get<long>(ll));
                break;
            case op::sub:
                rhs = calculate(op::sub, std::get<long>(ll), rhs);
                break;
            case op::mul:
                rhs = calculate(op::div, rhs, std::get<long>(ll));
                break;
            case op::div:
                rhs = calculate(op::mul, std::get<long>(ll), rhs);
                break;
            default:
                break;
        }
        l_expr = rr;
    }

    return solve_for_x(l_expr, rhs);
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

    auto monkeys = get_inputs(input);

    std::cout << "Part 1: " << std::endl;
    std::cout << evaluate(monkeys, "root") << std::endl;

    std::cout << "Part 2: " << std::endl;

    monkeys["humn"] = monkey_string_op{std::make_tuple(op::var, "humn", "humn")};

    auto [opr, lhs, rhs] = std::get<std::tuple<op, std::string, std::string>>(monkeys.at("root"));
    monkeys["root"] = monkey_string_op{std::make_tuple(op::eql, lhs, rhs)};

    auto result = build_expression(monkeys, "root");
    auto v = std::get<std::vector<monkey_expr>>(result);

    auto l_expr = v[1];
    auto r_expr = v[2];

    if (std::holds_alternative<long>(l_expr))
        std::swap(l_expr, r_expr);

    long r = std::get<long>(r_expr);

    std::cout << solve_for_x(l_expr, r) << std::endl;

    return 0;
}