#include <set>
#include <stack>
#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <limits>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

struct Point3D
{
    int x;
    int y;
    int z;

    Point3D operator+(const Point3D& rhs) const {
        return {x + rhs.x, y + rhs.y, z + rhs.z};
    }

    Point3D operator-(const Point3D& rhs) const {
        return {x - rhs.x, y - rhs.y, z - rhs.z};
    }

    std::vector<int> to_vector() const {
        return {x, y, z};
    }

    std::vector<Point3D> neighbours() const {
        const std::vector<Point3D> dirs = {{-1,0,0}, {1,0,0}, {0,-1,0}, {0,1,0}, {0,0,-1}, {0,0,1}};
        std::vector<Point3D> result = dirs 
                                    | rv::transform([this](const auto& p) { return *this + p; }) 
                                    | ranges::to<std::vector<Point3D>>();
        return result;                                
    }

    Point3D(std::vector<int> v) : x(v[0]), y(v[1]), z(v[2]) {}
    Point3D(int x, int y, int z) : x(x), y(y), z(z) {}

    friend std::ostream& operator<<(std::ostream& os, const Point3D& p) {
        os << "(" << p.x << ", " << p.y << ", " << p.z << ")";
        return os;
    }
};

auto operator<=>(const Point3D& lhs, const Point3D& rhs) {
    if (lhs.x < rhs.x) return std::strong_ordering::less;
    if (lhs.x > rhs.x) return std::strong_ordering::greater;
    if (lhs.y < rhs.y) return std::strong_ordering::less;
    if (lhs.y > rhs.y) return std::strong_ordering::greater;
    return lhs.z <=> rhs.z;
}

class BoulderMap {
    std::set<Point3D> boulders;
    int n_faces = 0;
    std::vector<int> mins = {INT_MAX, INT_MAX, INT_MAX};
    std::vector<int> maxs = {INT_MIN, INT_MIN, INT_MIN};

    bool outside(const Point3D& p) const {
        const auto pv = p.to_vector();
        for (const auto &[dim, coord] : pv | rv::enumerate) {
            if (coord < mins[dim] || coord > maxs[dim]) {
                return true;
            }
        }
        return false;
    }

    std::map<Point3D, bool> memo_path_to_outside;

    bool path_to_outside(const Point3D& p) {
        // is there a constant-coordinate path from the empty point p to the outside?
        std::set<Point3D> visited;
        std::stack<Point3D> to_visit;
        bool path_to_outside = false;

        to_visit.push(p);
        while (!to_visit.empty()) {
            Point3D q = to_visit.top();
            to_visit.pop();

            if (visited.contains(q)) {
                continue;
            }

            if (memo_path_to_outside.contains(q)) {
                path_to_outside = memo_path_to_outside.at(q);
                break;
            }

            if (outside(q)) {
                path_to_outside = true;
                break;
            }

            for (const auto& n : q.neighbours()) {
                if (!boulders.contains(n) && !visited.contains(n)) {
                    to_visit.push(n);
                }
            }
        }

        for (const auto& v : visited) {
            memo_path_to_outside[v] = path_to_outside;
        }

        return path_to_outside;
    }

    public:
        BoulderMap() = default;

        void add_boulder(const Point3D& p) {
            // update exposed faces
            int n_to_add = 6;
            for (const auto& n : p.neighbours()) {
                if (boulders.contains(n)) {
                    n_to_add -= 2;
                }
            }
            boulders.insert(p);
            n_faces += n_to_add;

            // update mins and maxs
            const auto pv = p.to_vector();
            for (const auto &[dim, coord] : pv | rv::enumerate) {
                mins[dim] = std::min(mins[dim], coord);
                maxs[dim] = std::max(maxs[dim], coord);
            }
        }

        int get_n_faces() const {
            return n_faces;
        }

        int get_n_external_faces() {
            int n_external = 0;
            for (const auto& p : boulders) {
                for (const auto& n : p.neighbours()) {
                    if (boulders.contains(n)) continue;

                    if (path_to_outside(n)) 
                        n_external++;
                    
                }
            }
            return n_external;
        }
};


std::vector<Point3D> get_inputs(std::istream &input) {
    std::vector<std::string> lines;

    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) 
            lines.push_back(line);
    }

    return lines | rv::transform([](const std::string& line) {
                                    const auto parts = line | rv::split(',') 
                                                            | rv::transform([](auto &&rng) {return std::string_view(&*rng.begin(), ranges::distance(rng));})
                                                            | rv::transform([](const auto& s) { 
                                                                return std::stoi(std::string(s)); }) 
                                                            | ranges::to<std::vector<int>>();
                                    return parts;
                                   }) 
                 | rv::transform([](const auto& v) { return Point3D{v[0], v[1], v[2]}; })
                 | ranges::to<std::vector<Point3D>>();
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

    const auto boulders = get_inputs(input);

    BoulderMap map;
    for (const auto& b : boulders) {
        map.add_boulder(b);
    }

    std::cout << "Part 1" << std::endl;
    std::cout << map.get_n_faces() << std::endl;

    std::cout << "Part 2" << std::endl;
    std::cout << map.get_n_external_faces() << std::endl;

    return 0;
}
