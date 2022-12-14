#include <vector>
#include <map>
#include <compare>
#include <string>
#include <optional>
#include <string_view>
#include <sstream>
#include <fstream>
#include <iostream>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

struct Point {
    int x;
    int y;

    Point diagonal_left() const { return Point{x-1, y+1}; }
    Point diagonal_right() const { return Point{x+1, y+1}; }
    Point below() const { return Point{x, y+1}; }

    Point(int x, int y) : x(x), y(y) {};

    Point(std::string s) {
        std::stringstream ss(s);
        std::string xstr, ystr;
        getline(ss, xstr, ',');
        getline(ss, ystr, ',');
        x = std::stoi(xstr);
        y = std::stoi(ystr);
    }

    Point operator+(const Point& rhs) const {
        return Point{x+rhs.x, y+rhs.y};
    }

    Point towards(const Point& rhs) const {
        Point result{0, 0};
        if (rhs.y != y) {
            result.y = rhs.y > y ? 1 : -1;
        }
        if (rhs.x != x) {
            result.x = rhs.x > x ? 1 : -1;
        }
        return result;
    }

    friend std::ostream& operator<<(std::ostream& os, const Point& p) {
        os << "(" << p.x << ", " << p.y << ")";
        return os;
    }

    bool operator!=(const Point& rhs) const {
        return x != rhs.x || y != rhs.y;
    }
};

auto operator <=>(const Point& lhs, const Point& rhs) {
    if (lhs.y < rhs.y) {
        return std::strong_ordering::less;
    } else if (lhs.y > rhs.y) {
        return std::strong_ordering::greater;
    } else {
        return lhs.x <=> rhs.x;
    }
}

enum struct map_element { WALL, SAND };

class Cave {
    std::map<Point, map_element> map;
    Point inlet;
    int deepest_wall;
    std::optional<int> floor;

    bool is_free(const Point& p) const {
        if (floor.has_value() && p.y == floor.value())
            return false;

        return map.find(p) == map.end();
    }

    std::optional<Point> next_sand_point() const {
        Point p = inlet;
        bool at_rest = false;
        do {
            std::vector<Point> next_points = {p.below(), p.diagonal_left(), p.diagonal_right()};
            std::vector<Point> free = next_points
                                    | rv::transform([this](const auto& p){
                                                         bool free = this->is_free(p);
                                                         return std::make_pair(p, free);})
                                    | rv::filter([](const auto& p){return p.second;})
                                    | rv::transform([](const auto& p){return p.first;})
                                    | ranges::to<std::vector>();

            if (free.empty()) {
                at_rest = true;
            } else {
                p = free[0];
            }
        } while (!at_rest && p.y <= deepest_wall);

        if (p.y > deepest_wall) {
            return std::nullopt;
        }
        return p;
    }

public:
    Cave(std::vector<std::pair<Point, Point>> wall_lines) : inlet(Point{500, 0}) {
        for (const auto& [p1, p2] : wall_lines) {
            auto dp = p1.towards(p2);
            for (auto p = p1; p != p2; p = p + dp) {
                map[p] = map_element::WALL;
            }
            map[p2] = map_element::WALL;
        }
        auto max_y = std::max_element(wall_lines.begin(), wall_lines.end(), [](const auto& lhs, const auto& rhs){return lhs.first.y < rhs.first.y;});
        deepest_wall = max_y->first.y;
    }

    std::pair<Point, Point> bounding() const {
        auto [min_x, max_x] = std::minmax_element(map.begin(), map.end(), [](const auto& lhs, const auto& rhs){return lhs.first.x < rhs.first.x;});
        auto [min_y, max_y] = std::minmax_element(map.begin(), map.end(), [](const auto& lhs, const auto& rhs){return lhs.first.y < rhs.first.y;});
        return std::make_pair(Point(min_x->first.x, min_y->first.y), Point(max_x->first.x, max_y->first.y));
    }

    void set_floor(int y) {
        floor = y;
        deepest_wall = y;
    }

    friend std::ostream& operator<<(std::ostream& os, const Cave& cave) {
        const auto ppt = cave.bounding();
        const auto &[min_xy, max_xy] = ppt;
        const auto &[min_x, min_y] = min_xy;
        const auto &[max_x, max_y] = max_xy;

        for (int y = min_y; y <= max_y; ++y) {
            std::string row = std::to_string(y);
            auto map_line = rv::iota(min_x, max_x)
                          | rv::transform([&cave, y](int x){return cave.map.find(Point{x, y});})
                          | rv::transform([&cave](const auto &it) {
                                if (it == cave.map.end()) {
                                    return ' ';
                                }
                                else return it->second == map_element::WALL ? '#' : 'o';
                            })
                          | ranges::to<std::string>();

            os << row << " " << map_line << std::endl;
        }
        return os;
    };

    bool add_sand() {
        if (!is_free(inlet))
            return false;

        auto ptest = next_sand_point();
        if (!ptest.has_value()) {
            return false;
        }

        auto p = ptest.value();
        map[p] = map_element::SAND;
        return true;
    }
};


std::vector<std::pair<Point, Point>> get_inputs(std::istream& input) {
    std::vector<std::pair<Point, Point>> lines;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty())
            continue;
        
        // can't figure out how to create a vector of pairs after the sliding...
        auto segments = line 
                   | rv::split(' -> ')
                   | rv::transform([](auto &&rng) {return std::string_view(&*rng.begin(), ranges::distance(rng));})
                   | rv::transform([](const auto &rng){return std::string(rng.begin(), rng.end());})
                   | rv::filter([](const std::string &s){return s != "->";})
                   | rv::transform([](const std::string& s){return Point(s);})
                   | rv::sliding(2)
                   | ranges::to<std::vector<std::vector<Point>>>();

        for (const auto& segement : segments) {
            lines.push_back(std::make_pair(segement[0], segement[1]));
        }
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

    auto inputs = get_inputs(input);
    Cave cave(inputs);

    int count = 0;
    while (cave.add_sand()) {
        count++;
    }

    std::cout << "Part 1" << std::endl;
    std::cout << count << std::endl;

    Cave cave2(inputs);
    auto bounds = cave2.bounding();
    cave2.set_floor(bounds.second.y+2);

    count = 0;
    while (cave2.add_sand()) {
        count++;
    }

    std::cout << "Part 2" << std::endl;
    std::cout << count << std::endl;
    return 0;
}