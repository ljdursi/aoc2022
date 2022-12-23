#include <map>
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
                                 | rv::to<std::set<Point>>();

        std::function<Point(const Point &, std::set<Point>&)> moves[4];

        moves[0] = [](p, occupied){if (!occupied.contains(N) && !occupied.contains(NE) && !occupied.contains(NW)) { return p + N; } return p;};
        moves[1] = [](p, occupied){if (!occupied.contains(S) && !occupied.contains(SE) && !occupied.contains(SW)) { return p + S; } return p;};
        moves[2] = [](p, occupied){if (!occupied.contains(W) && !occupied.contains(NW) && !occupied.contains(SW)) { return p + W; } return p;};
        moves[3] = [](p, occupied){if (!occupied.contains(W) && !occupied.contains(NE) && !occupied.contains(SE)) { return p + E; } return p;};

        for (size_t i=round; i<round+4; ++i) {
            size_t move_rule = (i%4)
            auto new_pt = moves[move_rule](p, occupied);
            if (new_pt != p)
                return make_pair(new_pt, p);
        }
        return make_pair(p, p);
    }

public:
    ElfDiffusion(std::set<Point> elves) {

    }
    void move_round() {
        // get all the proposed moves
        auto moves = map | rv::transform([&](const auto &p) { return proposed_move(p, round); })
                         | rv::to<std::vector<std::pair<Point, Point>>>();

        // filter any moves where multiple elves want to move to the same spot
        auto repeated = moves | rv::transform([&](const auto &p) { const auto &[new_pt, old_pt] = p; return new_pt; })
                              | rv::to<std::multiset<Point>>();

        repeated = repeated | rv::filter([&](const auto &p) { return repeated.count(p) > 1; })
                            | rv::to<std::multiset<Point>>();

        moves = moves | rv::filter([&](const auto &p) { const auto &[new_pt, old_pt] = p; return !repeated.contains(new_pt); })
                      | rv::to<std::vector<std::pair<Point, Point>>>();
        
        // move the elves
        for (const auto &[new_pt, old_pt] : moves) {
            map.erase(old_pt);
            map.insert(new_pt);
        }
        round++;
    }

    friend std::ostream& operator<<(std::ostream &os, const ElfDiffusion &ed) {
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
}