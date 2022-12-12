#include <vector>
#include <fstream>
#include <iostream>
#include <set>
#include <map>
#include <deque>
#include <algorithm>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

struct Position {
    int x;
    int y;

    friend std::ostream& operator<<(std::ostream &os, const Position &p) {
        return os << "(" << p.x << ", " << p.y << ")";
    }

    bool operator <(const Position &other) const {
        return x < other.x || (x == other.x && y < other.y);
    }

    bool operator ==(const Position &other) const {
        return x == other.x && y == other.y;
    }
};

using grid_t = std::vector<std::vector<char>>;

class HeightMap {
    int n;
    int m;
    grid_t heights;
    Position start;
    Position end;

public:
    HeightMap(grid_t heights, Position start, Position end) : heights(heights), start(start), end(end) {
        n = heights.size();
        m = heights[0].size();

        this->heights[start.x][start.y] = 'a' - 1;
        this->heights[end.x][end.y] = 'z';
    }

    char operator[](const Position &p) const {
        return heights[p.x][p.y];
    }

    bool valid(const Position &p) const {
        return p.x >= 0 && p.x < n && p.y >= 0 && p.y < m;
    }

    bool valid_move(const Position &p, const Position &q) const {
        if (!valid(p) || !valid(q)) return false;
        return (heights[q.x][q.y] - heights[p.x][p.y]) >= -1;
    }

    std::pair<int, int> get_size() const {
        return {n, m};
    }
};

// I was hoping to turn this into an input range but after a few tries I realized I'll have to read mroe...

class map_adjacency_iterator {
    const HeightMap &map;
    const Position p;
    std::vector<Position> moves;
    size_t idx;
    
public:
    map_adjacency_iterator(const HeightMap &map, const Position &p) : map(map), p(p), idx(0) {
        std::vector<Position> deltas = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        moves = deltas
              | rv::transform([&](const Position &dp) { Position q = {p.x + dp.x, p.y + dp.y}; return q; })
              | rv::filter([&](const Position &q) { return map.valid_move(p, q); })
              | ranges::to<std::vector<Position>>();
    }

    map_adjacency_iterator& operator++() {
        idx++;
        return *this;
    }

    Position operator*() const noexcept {
        return moves[idx];
    }

    map_adjacency_iterator begin() const {
        return *this;
    }

    map_adjacency_iterator end() const {
        map_adjacency_iterator it = *this;
        it.idx = moves.size();
        return it;
    }

    bool operator!=(const map_adjacency_iterator &other) const noexcept {
        if (p == other.p && idx == other.idx) return false;
        return true;
    }

    bool operator==(const map_adjacency_iterator &other) const noexcept { return !(*this != other); }
};


std::map<Position, int> BFS(const HeightMap &map, const Position &start, const Position &end) {
    std::map<Position, Position> parent;
    std::map<Position, int> distance;
    std::deque<Position> queue = {start};

    distance[start] = 0;

    while (!queue.empty()) {
        Position p = queue.front();
        queue.pop_front();

        for (const Position &q : map_adjacency_iterator(map, p)) {
            if (distance.contains(q) && distance[q] <= distance[p] + 1)
                continue;

            parent[q] = p;
            distance[q] = distance[p] + 1;
            queue.push_back(q);
        }
    }

    if (!parent.contains(end))
        return {};
    
    return distance;
}

std::tuple<Position, Position, grid_t> get_inputs(std::ifstream &input) {
    auto grid = ranges::getlines(input)
              | rv::transform([](const std::string &s) { std::vector<char> v(s.begin(), s.end()); return v; })
              | ranges::to<grid_t>();

    auto start_row = std::find_if(grid.begin(), grid.end(), [](const auto &v) {
        return std::find(v.begin(), v.end(), 'S') != v.end();
    });
    Position start = {(int)(start_row-grid.begin()),
                      (int)(std::find(start_row->begin(), start_row->end(), 'S') - start_row->begin())};

    auto end_row = std::find_if(grid.begin(), grid.end(), [](const auto &v) {
        return std::find(v.begin(), v.end(), 'E') != v.end();
    });
    Position end = {(int)(end_row-grid.begin()),
                    (int)(std::find(end_row->begin(), end_row->end(), 'E') - end_row->begin())};

    return std::make_tuple(start, end, grid);
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

    auto inputs = get_inputs(input);
    Position first = std::get<0>(inputs);
    Position last = std::get<1>(inputs);
    grid_t grid = std::get<2>(inputs);

    HeightMap hm(grid, first, last);
    std::set<Position> visited;

    auto distances = BFS(hm, last, first);
    std::cout << "Part 1" << std::endl;
    std::cout << distances[first] << std::endl;

    std::cout << "Part 2" << std::endl;
    auto &&[n, m] = hm.get_size();
    auto a_posns = rv::cartesian_product(rv::iota(0,n), rv::iota(0,m)) 
                 | rv::transform([](const auto &p) { const auto &[x, y] = p; return Position{x, y}; })
                 | rv::filter([&hm](const auto &p) { return hm[p] == 'a'; })
                 | ranges::to<std::vector<Position>>();

    auto a_distances = a_posns
                   | rv::transform([&distances](const auto &p) { return distances[p]; })
                   | rv::filter([](const auto &d) { return d != 0; })
                   | ranges::to<std::vector<int>>(); 

    std::cout << *std::min_element(a_distances.begin(), a_distances.end()) << std::endl;

    return 0;
}