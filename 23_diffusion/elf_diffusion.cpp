#include <set>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <compare>
#include <regex>
#include <algorithm>
#include <limits>
#include <cassert>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

struct Point {
    int x;
    int y;

    Point direction_to(const Point &other) const;
};

const Point N  = {-1,  0};
const Point NE = {-1, +1};
const Point E  = { 0, +1};
const Point SE = {+1, +1};
const Point S  = {+1,  0};
const Point SW = {+1, -1};
const Point W  = { 0, -1};
const Point NW = {-1, -1};

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

class ElfDiffusion {
    std::set<Point> map;
    int min_row = std::numeric_limits<int>::max();
    int min_col = std::numeric_limits<int>::max();
    int nrows;
    int ncols;

    int round = 0;

    std::pair<Point, Point> proposed_move(const Point &p, size_t round) const {
        auto adjacent = std::vector<Point>{N, NE, E, SE, S, SW, W, NW};
        auto occupied = adjacent | rv::transform([&](const auto &d) { return std::make_pair(d, p + d); })
                                 | rv::filter([&](const auto &p) { const auto &[d, new_pt] = p; return map.contains(new_pt); })
                                 | rv::transform([&](const auto &p) { const auto &[d, new_pt] = p; return d; })
                                 | ranges::to<std::set<Point>>();

        if (occupied.empty())
            return std::make_pair(p, p);

        std::function<Point(const Point &, std::set<Point>&)> moves[4];

        moves[0] = [](const Point &p, const std::set<Point> &occupied) {
                        if (!occupied.contains(N) && !occupied.contains(NE) && !occupied.contains(NW)) { return p + N; } return p;
                    };
        moves[1] = [](const Point &p, const std::set<Point> &occupied) {
                        if (!occupied.contains(S) && !occupied.contains(SE) && !occupied.contains(SW)) { return p + S; } return p;
                    };
        moves[2] = [](const Point &p, const std::set<Point> &occupied) {
                        if (!occupied.contains(W) && !occupied.contains(NW) && !occupied.contains(SW)) { return p + W; } return p;
                    };
        moves[3] = [](const Point &p, const std::set<Point> &occupied){
                        if (!occupied.contains(E) && !occupied.contains(NE) && !occupied.contains(SE)) { return p + E; } return p;
                    };

        for (size_t i=round; i<round+4; ++i) {
            size_t move_rule = i % 4;
            auto new_pt = moves[move_rule](p, occupied);
            if (new_pt != p)
                return std::make_pair(new_pt, p);
        }
        return std::make_pair(p, p);
    }

    void update_ranges() {
        auto minmax_x = std::minmax_element(map.begin(), map.end(), [](const auto &lhs, const auto &rhs) { return lhs.x < rhs.x; });
        auto minmax_y = std::minmax_element(map.begin(), map.end(), [](const auto &lhs, const auto &rhs) { return lhs.y < rhs.y; });

        min_row = minmax_x.first->x;
        nrows = minmax_x.second->x - min_row + 1;

        min_col = minmax_y.first->y;
        ncols = minmax_y.second->y - min_col + 1;
    }

public:
    ElfDiffusion(std::set<Point> elves) : map(elves) {
        update_ranges();
    }

    bool move_round() {
        // get all the proposed moves
        auto moves = map | rv::transform([&](const auto &p) { return proposed_move(p, round); })
                         | rv::filter([&](const auto &p) { const auto &[new_pt, old_pt] = p; return new_pt != old_pt; })
                         | ranges::to<std::vector<std::pair<Point, Point>>>();

        //std::cout << "Round " << round << " moves: " << moves.size() << std::endl;
        //for (const auto &[new_pt, old_pt] : moves) {
        //    std::cout << "  " << old_pt << " -> " << new_pt << std::endl;
        //}
        if (moves.empty())
            return true;

        // filter any moves where multiple elves want to move to the same spot
        auto repeated = moves | rv::transform([&](const auto &p) { const auto &[new_pt, old_pt] = p; return new_pt; })
                              | ranges::to<std::multiset<Point>>();

        repeated = repeated | rv::filter([&](const auto &p) { return repeated.count(p) > 1; })
                            | ranges::to<std::multiset<Point>>();

        moves = moves | rv::filter([&](const auto &p) { const auto &[new_pt, old_pt] = p; return !repeated.contains(new_pt); })
                      | ranges::to<std::vector<std::pair<Point, Point>>>();
        
        // move the elves
        for (const auto &[new_pt, old_pt] : moves) {
            map.erase(old_pt);
            map.insert(new_pt);
        }

        update_ranges();
        round++;

        return false;
    }

    size_t empty_ground_tiles() const {
        return nrows * ncols - map.size();
    }

    friend std::ostream& operator<<(std::ostream &os, const ElfDiffusion &ed) {
        //std::cout << ed.map.size() << std::endl;
        for (int row=ed.min_row; row<ed.min_row+ed.nrows; ++row) {
            for (int col=ed.min_col; col<ed.min_col+ed.ncols; ++col) {
                if (ed.map.contains(Point{row, col}))
                    os << "#";
                else
                    os << ".";
            }
            os << std::endl;
        }
        return os;
    }
};

std::set<Point> get_inputs(std::istream &input) {
    std::vector<std::string> lines;

    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty())
            lines.push_back(line);
    }

    auto map = lines 
        | rv::enumerate
        | rv::transform([](const auto &p) {
                        size_t row = p.first;
                        std::string line = p.second;
                        auto elements = line | rv::enumerate
                                             | rv::filter([](const auto &p) { return p.second == '#'; })
                                             | rv::transform([row](const auto &p) { return Point{(int)row, (int)p.first}; })
                                             | ranges::to<std::vector<Point>>();
                        return elements; })
        | rv::join
        | ranges::to<std::set<Point>>();

    return map;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <seams_file>" << std::endl;
        return 1;
    }

    std::ifstream input(argv[1]);
    if (!input.is_open()) {
        std::cerr << "Could not open input file " << argv[1] << std::endl;
        return 2;
    }

    const auto map = get_inputs(input);
    ElfDiffusion ed(map);

    std::cout << ed << std::endl;

    int round;
    for (round=0; round<10; ++round) {
        ed.move_round();
        std::cout << ed << std::endl;
        std::cout << std::endl;
    }

    std::cout << "Part 1" << std::endl;
    std::cout << ed.empty_ground_tiles() << std::endl;

    while (!ed.move_round()) {
        round++;
    }

    std::cout << "Part 2" << std::endl;
    std::cout << round+1 << std::endl;

    return 0;
}