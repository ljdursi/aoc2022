#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <compare>
#include <regex>
#include <algorithm>
#include <limits>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

struct Point {
    int x;
    int y;
};

auto operator<=>(const Point &lhs, const Point &rhs) {
    if (lhs.x < rhs.x) return std::strong_ordering::less;
    if (lhs.x > rhs.x) return std::strong_ordering::greater;
    return lhs.y <=> rhs.y;
}

auto operator==(const Point &lhs, const Point &rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

Point operator+(const Point &lhs, const Point &rhs) {
    return Point{lhs.x + rhs.x, lhs.y + rhs.y};
}

std::ostream &operator<<(std::ostream &os, const Point &p) {
    os << "(" << p.x << ", " << p.y << ")";
    return os;
}

enum struct Map_element : char { empty = '.', wall = '#' };

const std::map<char, Map_element> valid_elements = {
    {'.', Map_element::empty},
    {'#', Map_element::wall},
};

enum struct Turn_dir : char { left = 'L', right = 'R', straight = 'S' };

const std::map<char, Turn_dir> valid_turn_dirs = {
    {'L', Turn_dir::left},
    {'R', Turn_dir::right},
    {'S', Turn_dir::straight},
};

class Orientation {
    // in direction of turning right (clockwise)
    const std::vector<Point> dirs = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    const size_t ndirs = dirs.size();
    size_t dir_idx = 0;

    public:
        Orientation() = default;
        Point dir() const { return dirs[dir_idx]; }
        int facing() const { return (int)dir_idx; }

        void turn(Turn_dir turn_dir) {
            switch (turn_dir) {
                case Turn_dir::left:     dir_idx = (dir_idx + (ndirs-1)) % ndirs; break;
                case Turn_dir::right:    dir_idx = (dir_idx + 1) % ndirs; break;
                case Turn_dir::straight: break;
                default: break;
            }
        }
};

struct Move {
    int n_steps;
    Turn_dir turn_dir;        
};

std::ostream &operator<<(std::ostream &os, const Move &m) {
    os << m.n_steps << " " << static_cast<char>(m.turn_dir);
    return os;
}

class Navigator {
    std::map<Point, Map_element> map;
    std::map<Point, Point> next_point;
    Orientation orientation;
    Point position;
    int nrows;
    int ncols;

public:
    Navigator(std::map<Point, Map_element> map) : map{map} {
        auto row_ranges = std::minmax_element(map.begin(), map.end(), [](const auto &lhs, const auto &rhs) { return lhs.first.x < rhs.first.x; });
        auto col_ranges = std::minmax_element(map.begin(), map.end(), [](const auto &lhs, const auto &rhs) { return lhs.first.y < rhs.first.y; });

        nrows = row_ranges.second->first.x + 1;
        ncols = col_ranges.second->first.y + 1;

        std::vector<std::pair<int, int>> row_boundaries;
        for (int row = 0; row < nrows; ++row)
            row_boundaries.push_back(std::make_pair(std::numeric_limits<int>::max(), std::numeric_limits<int>::min()));

        std::vector<std::pair<int, int>> col_boundaries;
        for (int col = 0; col < ncols; ++col)
            col_boundaries.push_back(std::make_pair(std::numeric_limits<int>::max(), std::numeric_limits<int>::min()));

        for (const auto &[p, e] : map) {
            const int row = p.x;
            const int col = p.y;

            if (col < row_boundaries[row].first) {
                row_boundaries[row].first = col;
            }
            if (col > row_boundaries[row].second) {
                row_boundaries[row].second = col;
            }

            if (row < col_boundaries[col].first) {
                col_boundaries[col].first = row;
            }
            if (row > col_boundaries[col].second) {
                col_boundaries[col].second = row;
            }
        }

        for (int row = 0; row < nrows; ++row) {
            Point left_boundary{row, row_boundaries[row].first};
            Point right_boundary{row, row_boundaries[row].second};
            next_point[right_boundary + Point{0, 1}] = left_boundary;
            next_point[left_boundary + Point{0, -1}] = right_boundary;
        }

        for (int col = 0; col < ncols; ++col) {
            Point top_boundary{col_boundaries[col].first, col};
            Point bottom_boundary{col_boundaries[col].second, col};
            next_point[bottom_boundary + Point{1, 0}] = top_boundary;
            next_point[top_boundary + Point{-1, 0}] = bottom_boundary;
        }

        position = Point{0, row_boundaries[0].first};
        while (map.at(position) != Map_element::empty) {
            position = position + Point{0, 1};
        }
    }

    void move(Move move) {
        Point dir = orientation.dir();
        for (int i = 0; i < move.n_steps; ++i) {
            Point next = position + dir;

            if (!map.contains(next))
                next = next_point.at(next);

            if (map.at(next) == Map_element::empty) {
                position = next;
            } else {
                break;
            }
        }
        orientation.turn(move.turn_dir);
    }

    int password() const {
        int row = position.x + 1;
        int col = position.y + 1;
        int facing = orientation.facing();

        return 1000 * row + 4 * col + facing;
    }

    friend std::ostream &operator<<(std::ostream &os, const Navigator &nav) {
        for (int row = 0; row < nav.nrows; ++row) {
            for (int col = 0; col < nav.ncols; ++col) {
                const Point p{row, col};
                if (nav.map.contains(p)) {
                    if (p == nav.position)
                        os << 'X';
                    else
                        os << static_cast<char>(nav.map.at(p));
                } else {
                    os << ' ';
                }
            }
            os << std::endl;
        }
        return os;
    }
};

std::pair<std::map<Point, Map_element>, std::vector<Move>> get_inputs(std::istream &input) {
    std::vector<std::string> lines;

    std::string line;
    while (std::getline(input, line))
        lines.push_back(line);
    
    auto map = lines 
        | rv::enumerate
        | rv::take_while([](const auto &p) { const std::string line = p.second; return !line.empty(); })
        | rv::transform([](const auto &p) {
                        size_t row = p.first;
                        std::string line = p.second;
                        auto elements = line | rv::enumerate
                                             | rv::filter([](const auto &p) { return p.second != ' '; })
                                             | rv::transform([row](const auto &p) { return std::make_pair(Point{(int)row, (int)p.first}, valid_elements.at(p.second)); })
                                             | ranges::to<std::vector<std::pair<Point, Map_element>>>();
                        return elements; })
        | rv::join
        | ranges::to<std::map<Point, Map_element>>();

    auto move_lines = lines
                    | rv::drop_while([](const std::string &line) { return !line.empty(); })
                    | rv::drop(1)
                    | ranges::to<std::vector<std::string>>();

    move_lines[0] = move_lines[0] + "S";
    std::vector<Move> moves;
    std::regex move_regex{"(\\d+)([LRS])"}; 
    std::sregex_iterator it(move_lines[0].begin(), move_lines[0].end(), move_regex);
    for (auto &i = it; i != std::sregex_iterator(); ++i) {
        std::smatch m = *it;
        moves.push_back(Move{std::stoi(m[1]), valid_turn_dirs.at(m[2].str()[0])});
    }

    return std::make_pair(map, moves);
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

    const auto &[map, moves] = get_inputs(input);
    Navigator nav{map};

    for (const auto &move : moves) {
        nav.move(move);
    }

    std::cout << "Part 1" << std::endl;
    std::cout << nav.password() << std::endl;

    return 0;
}