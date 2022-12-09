#include <set>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

class DeltaPosition {
public:
    DeltaPosition(int dx, int dy) : dx(dx), dy(dy) {}
    int dx;
    int dy;

    DeltaPosition operator+(const DeltaPosition &other) const {
        return DeltaPosition(dx + other.dx, dy + other.dy);
    }

    DeltaPosition operator+=(const DeltaPosition &other) {
        dx += other.dx;
        dy += other.dy;
        return *this;
    }

    bool is_leftward() const { return dx < 0; }
    bool is_rightward() const { return dx > 0; }
    bool is_upward() const { return dy < 0; }
    bool is_downward() const { return dy > 0; }
};

const DeltaPosition Up(0, -1);
const DeltaPosition Down(0, +1);
const DeltaPosition Left(-1, 0);
const DeltaPosition Right(+1, 0);
const DeltaPosition None(0, 0);

class Position {
private:
    int y;
    int x;

public:
    Position(int x, int y) : x(x), y(y) {}

    bool operator<(const Position &other) const {
        if (y < other.y) { return true; }
        if (y > other.y) { return false; }
        return x < other.x;
    }

    std::pair<int, int> get_position() const {
        return std::make_pair(x, y);
    }

    DeltaPosition delta(const Position &other) const {
        return DeltaPosition(other.x - x, other.y - y);
    }

    bool adjacent(const Position &other) const {
        auto delta = this->delta(other);
        if (std::abs(delta.dx) > 1 || std::abs(delta.dy) > 1) {
            return false;
        }
        return true;
    }

    void move(const DeltaPosition &delta) {
        x += delta.dx;
        y += delta.dy;
    }
};

class RopeBridge {
    std::vector<Position> knots;
    std::set<Position> tailpath;
    int nknots;

    const DeltaPosition knot_move(const Position &head, const Position &tail) const {
        if (head.adjacent(tail)) {
            return None;
        }

        DeltaPosition result = None;
        auto head_tail_delta = tail.delta(head);

        if (head_tail_delta.is_leftward())
            result += Left;
        if (head_tail_delta.is_rightward())
            result += Right;
        if (head_tail_delta.is_upward())
            result += Up;
        if (head_tail_delta.is_downward())
            result += Down;

        return result;
    }

public:
    RopeBridge(int npts) : knots(npts, {0, 0}), nknots(npts) { 
        tailpath.insert(knots[nknots-1]);
    }
    int n_posns() const { return tailpath.size(); }

    void move_head(const DeltaPosition &delta) {
        knots[0].move(delta);

        for (int i=1; i<nknots; ++i) {
            auto move = knot_move(knots[i-1], knots[i]);
            knots[i].move(move);
        }

        tailpath.insert(knots[nknots-1]);
    }

    std::vector<std::vector<char>> positions_to_grid() const {
        const auto minmax_x = ranges::minmax( knots | rv::transform([](const auto &pos) { return pos.get_position().first; }),
                                                std::less<int>() );
        const auto minmax_y = ranges::minmax( knots | rv::transform([](const auto &pos) { return pos.get_position().second; }),
                                                std::less<int>() );

        int xmin = std::min(0, minmax_x.min);
        int xmax = std::max(0, minmax_x.max);
        int ymin = std::min(0, minmax_y.min);
        int ymax = std::max(0, minmax_y.max);

        std::vector<std::vector<char>> grid(ymax - ymin + 1, std::vector<char>(xmax - xmin + 1, '.'));

        grid[0-ymin][0-xmin] = 's';

        for (auto &&p : knots | rv::enumerate | rv::reverse ) {
            auto &&[i, pos] = p;
            auto &&[tpos_x, tpos_y] = pos.get_position();
            grid[tpos_y-ymin][tpos_x-xmin] = i + '0';
        }
        auto hp = knots[0].get_position();
        auto tp = knots[nknots-1].get_position();

        grid[tp.second-ymin][tp.first-xmin] = 'T';
        grid[hp.second-ymin][hp.first-xmin] = 'H';

        return grid;
    }

    std::vector<std::vector<char>> tail_path_to_grid() const {
        auto tmin_x = INT_MAX, tmax_x = INT_MIN;
        auto tmin_y = INT_MAX, tmax_y = INT_MIN;

        for (auto &pos : tailpath) {
            const auto &&[tpos_x, tpos_y] = pos.get_position();

            tmin_x = std::min(tmin_x, tpos_x);
            tmax_x = std::max(tmax_x, tpos_x);
            tmin_y = std::min(tmin_y, tpos_y);
            tmax_y = std::max(tmax_y, tpos_y);
        }

        std::vector<std::vector<char>> grid(tmax_y - tmin_y + 1, std::vector<char>(tmax_x - tmin_x + 1, '.'));

        for (auto &pos : tailpath) {
            const auto &&[tpos_x, tpos_y] = pos.get_position();
            grid[tpos_y-tmin_y][tpos_x-tmin_x] = '#';
        }

        return grid;
    }
};

void print_grid(const std::vector<std::vector<char>> &grid) {
    for (auto &row : grid) {
        for (auto &col : row) {
            std::cout << col;
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

std::vector<std::tuple<char, DeltaPosition, int>> get_inputs(std::ifstream &input) {
    std::vector<std::tuple<char, DeltaPosition, int>> result;
    std::string line;

    while (std::getline(input, line)) {
        auto ss = std::stringstream(line);
        std::string dir;
        std::string npos;

        ss >> dir >> npos;
        DeltaPosition delta = None;
        switch (dir[0]) {
            case 'U':
                delta = Up;
                break;
            case 'D':
                delta = Down;
                break;
            case 'L':
                delta = Left;
                break;
            case 'R':
                delta = Right;
                break;
        }
        result.push_back(std::make_tuple(dir[0], delta, std::stoi(npos)));
    }

    return result;
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
    RopeBridge bridge(2);

    for (auto &input : inputs) {
        auto &&[dir, delta, npos] = input;
        for (int i = 0; i < npos; i++) {
            bridge.move_head(delta);
        }
    }

    std::cout << "Part 1: " << std::endl;
    std::cout << bridge.n_posns() << std::endl;

    RopeBridge bridge2(10);

    bool print = false;
    if (print) {
        std::cout << "== Initial State ==" << std::endl << std::endl;
        print_grid(bridge2.positions_to_grid());
    }

    for (auto &input : inputs) {
        auto &&[dir, delta, npos] = input;
        if (print) std::cout << "== " << dir << " " << npos << " ==" << std::endl << std::endl;
        for (int i = 0; i < npos; i++) {
            bridge2.move_head(delta);
            if (print) print_grid(bridge2.positions_to_grid());
        }
    }

    if (print) {
        std::cout << "== Tail Path ==" << std::endl << std::endl;
        print_grid(bridge2.tail_path_to_grid());
    }

    std::cout << "Part 2: " << std::endl;
    std::cout << bridge2.n_posns() << std::endl;

    return 0;
}