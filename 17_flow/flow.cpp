#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <compare>
#include <cassert>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

struct Point {
    long x;
    long y;

    Point operator+(const Point& rhs) const {
        return {x + rhs.x, y + rhs.y};
    }
};

const Point LEFT = {-1, 0};
const Point RIGHT = {+1, 0};
const Point DOWN = {0, -1};
const Point UP = {0, +1};

auto operator <=> (const Point& lhs, const Point& rhs) {
    if (rhs.y > lhs.y) return std::strong_ordering::greater;
    if (rhs.y < lhs.y) return std::strong_ordering::less;
    return rhs.x <=> lhs.x;
}

using rock_block = std::vector<std::vector<char>>;

// These blocks are defined "bottom up" - e.g. row 0 is the bottom row
// (that only really matters for the ell shape)

const rock_block minus = {{'@', '@', '@', '@'}};

const rock_block plus  = {{' ', '@', ' '},
                          {'@', '@', '@'},
                          {' ', '@', ' '}};

const rock_block ell  = {{'@', '@', '@'},
                         {' ', ' ', '@'},
                         {' ', ' ', '@'}};

const rock_block pipe = {{'@'},
                         {'@'},
                         {'@'},
                         {'@'}};

const rock_block square = {{'@', '@'},
                           {'@', '@'}};

class Tetris {
    const int width = 7;
    long max_height = -1;

    int current_block = -1;
    const std::vector<rock_block> blocks = {minus, plus, ell, pipe, square};
    const size_t n_block_types = blocks.size();
    bool block_in_motion = false;

    std::string winds = "";
    int wind_idx = -1;

    std::set<Point> map;

    bool is_blocked(const Point &block_position, const rock_block &block, const Point &direction) {
        for (const auto &[j, block_row]: block | rv::enumerate) {
            for (const auto &[i, block_elem]: block_row | rv::enumerate) {

                if (block_elem == ' ') 
                    continue;
                
                Point position = block_position + Point{(int)i, (int)j};
                Point new_position = position + direction;

                assert(!map.contains(position));
                if (map.contains(new_position)) {
                    return true;
                }

                if ((new_position.y < 0) || (new_position.x >= width) || (new_position.x < 0)) {
                    return true;
                }
            }
        }
        return false;
    }

    Point wind_direction() {
        wind_idx++;
        if (wind_idx >= winds.size()) 
            wind_idx = 0;

        char wind = winds[wind_idx];
        assert((wind == '<') || (wind == '>'));
        switch (wind) {
            case '<': return LEFT;
            case '>': return RIGHT;
        }

        return {0, 0};
    }
    
    rock_block next_block() {
        current_block++;
        if (current_block >= n_block_types) 
            current_block = 0;
        return blocks[current_block];
    }

    std::set<Point> last_five() const {
        std::set<Point> last_five;
        for (int y = 0; y < 5; y++) {
            for (int x = 0; x < width; x++) {
                if (map.contains(Point{(long)x, (long)(max_height-y)}))
                    last_five.insert(Point{(long)x, (long)y});
            }
        }
        return last_five;
    }   

public:
    void add_block() {
        rock_block block = next_block();
        Point block_position = {2, max_height + 4};

        block_in_motion = true;

        do {
            auto wind = wind_direction();
            if (!is_blocked(block_position, block, wind)) {
                block_position = block_position + wind;
            }
            
            if (is_blocked(block_position, block, DOWN)) {
                block_in_motion = false;
            } else {
                block_position = block_position + DOWN;
            }
        } while (block_in_motion);

        for (const auto &[j, block_row]: block | rv::enumerate) {
            for (const auto &[i, block_elem]: block_row | rv::enumerate) {
                if (block_elem != ' ') {
                    Point position = block_position + Point{(long)i, (long)j};
                    map.insert(position);
                    max_height = std::max(max_height, position.y);
                }
            }
        }
    }

    Tetris(const std::string &winds) : winds(winds) {}

    auto state() const {
        return std::make_tuple(current_block, wind_idx, last_five());
    }

    long get_max_height() const {
        return max_height;
    }

    friend std::ostream& operator<<(std::ostream &os, const Tetris &tetris) {
        for (long y = tetris.max_height; y >= 0; y--) {
            std::cout << "|";
            for (int x = 0; x < tetris.width; x++) {
                if (tetris.map.contains(Point{(long)x, (long)y})) {
                    os << '#';
                } else {
                    os << '.';
                }
            }
            os << "|" << std::endl;
        }
        std::cout << "|" << std::string(tetris.width, '-') << "|" << std::endl;
        return os;
    }
};

std::string get_inputs(std::istream &input) {
    std::vector<std::string> lines;

    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) 
            lines.push_back(line);
    }

    return lines[0];
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

    const auto wind = get_inputs(input);

    Tetris tetris(wind);

    long i;
    for (i = 0; i < 2022; i++) {
        tetris.add_block();
    }

    std::cout << tetris.get_max_height()+1 << std::endl;

    std::map<std::tuple<int, int, std::set<Point>>, std::pair<int, long>> seen;
    bool cycle_found = false;
    long cycle_len = 0, height_diff = 0;

    while (!cycle_found) {
        tetris.add_block();
        i++;

        auto state = tetris.state();
        if (seen.contains(state)) {
            auto &[cycle_start, height_start] = seen[state];
            cycle_len = i - cycle_start;
            height_diff = tetris.get_max_height() - height_start;
            cycle_found = true;
            std::cout << "Cycle found at height " << tetris.get_max_height() << std::endl;
        } else {
            seen[state] = {i, tetris.get_max_height()};
        }
    }

    long long height = tetris.get_max_height();
    std::cout << "Cycle found at " << i << " with length " << cycle_len << " and height diff " << height_diff << std::endl;

    // we will now skip ahead enough cycles to get close to 1 trillion
    long ncycles = ((1000000000000L - 1L) - i) / cycle_len;
    height += height_diff * ncycles;
    i += cycle_len * ncycles;

    std::cout << "Skipping ahead to " << i << " (" << ncycles << " cycles of " << cycle_len << ")" << std::endl;

    auto old_height = tetris.get_max_height();
    for (;i<1000000000000; i++) {
        tetris.add_block();
    }
    height += (tetris.get_max_height() - old_height);

    std::cout << height+1 << std::endl;

    return 0;
}