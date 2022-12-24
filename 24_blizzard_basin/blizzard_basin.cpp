#include <set>
#include <map>
#include <vector>
#include <queue>
#include <string>
#include <fstream>
#include <iostream>
#include <compare>
#include <algorithm>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

struct Point {
    int x;
    int y;

    Point direction_to(const Point &other) const;
};

const Point N  = {-1,  0};
const Point E  = { 0, +1};
const Point S  = {+1,  0};
const Point W  = { 0, -1};
const Point STAY = { 0, 0};


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

Point operator*(const Point &lhs, int rhs) {
    return Point{lhs.x * rhs, lhs.y * rhs};
}

Point rotate(const Point &p, int nrows, int ncols) {
    Point q = p;
    while (q.x < 0) {
        q.x += nrows;
    }
    while (q.x >= nrows) {
        q.x -= nrows;
    }
    while (q.y < 0) {
        q.y += ncols;
    }
    while (q.y >= ncols) {
        q.y -= ncols;
    }
    return q;
}

std::ostream &operator<<(std::ostream &os, const Point &p) {
    os << "(" << p.x << ", " << p.y << ")";
    return os;
}

const std::map<char, Point> blizzard_to_dir = { {'^', N}, {'<', W}, {'v', S}, {'>', E}, };
const std::map<Point, char> dir_to_blizzard = { {N, '^'}, {W, '<'}, {S, 'v'}, {E, '>'}, };

struct Blizzard {
    const Point direction;
    Point start;

    friend std::ostream &operator<<(std::ostream &os, const Blizzard &b) {
        os << "Blizzard: " << b.start << " moving in direction " << b.direction;
        return os;
    }
};

struct BlizzardsState {
    std::vector<Blizzard> blizzards;
    size_t nrows;
    size_t ncols;
    Point start;
    Point goal;
};

std::vector<std::vector<std::vector<char>>> maps_from_blizards(const BlizzardsState &bs) {
    std::vector<Blizzard> blizzards = bs.blizzards;
    size_t nrows = bs.nrows;
    size_t ncols = bs.ncols;

    size_t nminutes = nrows * ncols;
    std::vector<std::vector<std::vector<char>>> maps(nminutes, std::vector<std::vector<char>>(nrows+2, std::vector<char>(ncols+2, '.')));

    for (size_t minute=0; minute<nminutes; ++minute) {
        // walls
        for (size_t row=0; row<nrows+2; ++row) {
            maps[minute][row][0] = '#';
            maps[minute][row][ncols+1] = '#';
        }
        for (size_t col=1; col<ncols+1; ++col) {
            maps[minute][0][col] = '#';
            maps[minute][nrows+1][col] = '#';
        }
        maps[minute][bs.start.x][bs.start.y] = '.';
        maps[minute][bs.goal.x][bs.goal.y] = '.';
        
        // blizzards
        for (auto &blizzard : blizzards) {
            Point p = blizzard.start + Point{-1, -1};
            p = rotate(p + blizzard.direction * (int)minute, nrows, ncols); 
            p = p + Point{1, 1};

            char existing = maps[minute][p.x][p.y];

            if (existing != '.' && existing != '#') {
                if (existing > '1' && existing < '4') {
                    maps[minute][p.x][p.y] = existing + (char)1;
                } else {
                    maps[minute][p.x][p.y] = '2';
                }
            } else {
                maps[minute][p.x][p.y] = dir_to_blizzard.at(blizzard.direction);
            }
        }
    }

    return maps;
}

std::vector<Point> bfs(const std::vector<std::vector<std::vector<char>>> &maps, const BlizzardsState &bs, int initial_minute=0) {
    const size_t nminutes = maps.size();

    using coordinates = std::tuple<Point, int>;
    std::set<coordinates> visited;

    using state = std::tuple<std::vector<Point>, int>;
    std::queue<state> que;
    que.push({{bs.start}, initial_minute});

    while (!que.empty()) {
        auto [path, minute] = que.front();
        que.pop();

        Point p = path.back();

        if (p == bs.goal) {
            return path;
        }

        if (visited.contains({p, minute}))
            continue;

        for (auto dir : {S, E, N, W, STAY}) {
            Point q = p + dir;

            if (q.x < 0 || q.x >= bs.nrows+2 || q.y < 0 || q.y >= bs.ncols+2) 
                continue;

            if (maps[(minute+1) % nminutes][q.x][q.y] == '.') {
                std::vector<Point> new_path = path;
                new_path.push_back(q);
                que.push({new_path, minute+1});
            }
        }
        visited.insert({p, minute});
    }

    return {};
}

BlizzardsState get_inputs(std::istream &input) {
    std::vector<std::string> lines;

    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty())
            lines.push_back(line);
    }

    BlizzardsState result;
    result.nrows = lines.size() - 2;    
    result.ncols = lines[0].size() - 2;

    result.start = Point{0, (int)lines[0].find('.')};
    result.goal = {(int)(lines.size()-1), (int)(lines[lines.size()-1].find('.'))};

    result.blizzards = lines | rv::enumerate
                             | rv::drop(1)
                             | rv::take(result.nrows)
                             | rv::transform([](const auto &p) {
                                                const auto &[i, line] = p;
                                                auto blizzards = line | rv::enumerate
                                                                      | rv::filter([](const auto &p2) { return (p2.second != '.' && p2.second != '#'); })
                                                                      | rv::transform([i=i](const auto &p2) { return Blizzard{blizzard_to_dir.at(p2.second), Point{(int)i, (int)p2.first}}; })
                                                                      | ranges::to<std::vector<Blizzard>>();
                                                return blizzards;
                                             })
                             | rv::join
                             | ranges::to<std::vector<Blizzard>>();

    return result;
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

    auto blizzards_state = get_inputs(input);
    const auto maps = maps_from_blizards(blizzards_state);

    auto best_path = bfs(maps, blizzards_state);

    std::cout << "Part 1" << std::endl;
    std::cout << best_path.size()-1 << std::endl;

    std::cout << "Part 2" << std::endl;
    int minute = best_path.size()-1;

    std::swap(blizzards_state.start, blizzards_state.goal);
    auto path_back = bfs(maps, blizzards_state, minute);
    int minute2 = minute + path_back.size()-1;

    std::swap(blizzards_state.start, blizzards_state.goal);
    auto return_path = bfs(maps, blizzards_state, minute2);

    std::cout << minute2 + return_path.size()-1 << std::endl;

    return 0;
}