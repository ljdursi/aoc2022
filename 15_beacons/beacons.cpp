#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <regex>
#include <map>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

struct Point {
    int x;
    int y;
    Point(int x, int y) : x(x), y(y) {};
    int manhattan_distance(const Point& rhs) const {
        return std::abs(x - rhs.x) + std::abs(y - rhs.y);
    }

    friend std::ostream& operator<<(std::ostream& os, const Point& p) {
        os << "(" << p.x << ", " << p.y << ")";
        return os;
    }
};

auto operator <=> (const Point& lhs, const Point& rhs) {
    if (lhs.y < rhs.y) {
        return std::strong_ordering::less;
    } else if (lhs.y > rhs.y) {
        return std::strong_ordering::greater;
    } else {
        return lhs.x <=> rhs.x;
    }
}

struct range {
    int y;
    int start, end;

    range(int y, int start, int end) : y(y), start(start), end(end) {};

    bool overlaps(const range& rhs) const {
        if (y != rhs.y)
            return false;

        return start <= rhs.end && end >= rhs.start;
    }

    bool adjacent(const range& rhs) const {
        if (y != rhs.y)
            return false;

        return start == rhs.end + 1 || end == rhs.start - 1;
    }

    bool empty() const {
        return start > end;
    }

    int size() const {
        return end - start + 1;
    }

    friend std::ostream& operator<<(std::ostream& os, const range& r) {
        os << r.y << ": (" << r.start << ", " << r.end << ")";
        return os;
    }

    std::optional<range> merge(const range& rhs) const {
        if (overlaps(rhs) || adjacent(rhs))
            return range{y, std::min(start, rhs.start), std::max(end, rhs.end)};
        else 
            return {};
    }

    std::vector<range> operator-(const range& rhs) const {
        if (!overlaps(rhs))
            return {*this};
            
        std::vector<range> result;

        if (start <= rhs.start) {
            range r0{y, start, rhs.start - 1};
            if (!r0.empty())
                result.push_back(r0);

            range r1{y, rhs.end+1, end};
            if (!r1.empty())
                result.push_back(r1);  
        } else {
            range r0{y, rhs.end+1, end};
            if (!r0.empty())
                result.push_back(r0);
        }

        return result;
    }

    range intersection(const range& rhs) const {
        if (!overlaps(rhs))
            return range{y, 0, -1};

        return range{y, std::max(start, rhs.start), std::min(end, rhs.end)};
    }



    std::optional<range> remove_point_on_boundary(const Point& p) const {
        if (p.y != y)
            return {};

        if (p.x == start && p.x == end)
            return {};
        else if (p.x == start)
            return range{y, start+1, end};
        else if (p.x == end)
            return range{y, start, end-1};
        else
            return {};
    }

    bool operator<(const range& rhs) const {
        if (y < rhs.y) {
            return true;
        } else if (y > rhs.y) {
            return false;
        }

        return start < rhs.start;
    }
};

range intersection(const std::vector<range>& ranges) {
    if (ranges.empty())
        return range{0, 0, -1};

    range result=ranges[0];

    for (auto r : ranges | rv::drop(1)) {
        result = result.intersection(r);
    }

    return result;
}

std::map<int, std::vector<range>> merge_ranges(const std::vector<range> &ranges) {
    std::map<int, std::vector<range>> ranges_by_y;

    for (auto r : ranges) {
        ranges_by_y[r.y].push_back(r);
    }

    for (auto& [y, ranges] : ranges_by_y) {
        std::stack<range> stack;

        std::sort(ranges.begin(), ranges.end());
        stack.push(ranges[0]);

        for (const auto &[i, r] : ranges | rv::enumerate | rv::drop(1)) {
            if (stack.top().overlaps(r) || stack.top().adjacent(r)) {
                auto merged = stack.top().merge(r);
                stack.pop();
                stack.push(*merged);
            } else {
                stack.push(r);
            }
        }

        ranges.clear();
        while (!stack.empty()) {
            ranges.push_back(stack.top());
            stack.pop();
        }
    }

    return ranges_by_y;
}

class Sensor {
    Point position;
    Point beacon;
    int exclusion_distance;

public:
    Sensor(Point position, Point beacon) : position(position), beacon(beacon) {
        exclusion_distance = position.manhattan_distance(beacon);
    };

    std::optional<range> exclusion_range(const int y, bool exclude_my_beacon = true) const {
        if (y < position.y - exclusion_distance || y > position.y + exclusion_distance)
            return {};

        int d = exclusion_distance - std::abs(y - position.y);
        range r{y, position.x - d, position.x + d};

        if (y == beacon.y && exclude_my_beacon) {
            return r.remove_point_on_boundary(beacon);
        } else {
            return r;
        }
    }

    std::vector<range> exclusion_ranges(bool exclude_my_beacon=true) const {
        int min_y = position.y - exclusion_distance;
        int max_y = position.y + exclusion_distance;

        auto result = rv::iota(min_y, max_y + 1)
                    | rv::transform([this, exclude_my_beacon](int y) { return exclusion_range(y, exclude_my_beacon); })
                    | rv::filter([](const std::optional<range> &optr) { return optr.has_value(); })
                    | rv::transform([](const std::optional<range> &optr) { return optr.value(); })
                    | rv::filter([](const range &r) { return !r.empty(); })
                    | ranges::to<std::vector<range>>();

        return result;
    }

    friend std::ostream& operator<<(std::ostream& os, const Sensor& s) {
        os << s.position << " -> " << s.beacon << " (" << s.exclusion_distance << ")";
        return os;
    }
};

std::vector<Sensor> get_inputs(std::istream &input){
    std::vector<std::string> lines;

    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) 
            lines.push_back(line);
    }


    std::vector<Sensor> sensors = lines
                                | rv::transform([](const auto &line) {
                                                    std::vector<int> numbers;
                                                    std::regex re("(-?\\d+)");
                                                    std::smatch match;

                                                    for (std::sregex_iterator it(line.begin(), line.end(), re), end; it != end; ++it) {
                                                        numbers.push_back(std::stoi(it->str()));
                                                    }
                                                    return numbers;
                                                })
                                | rv::transform([](const auto &numbers) {
                                                    return Sensor(Point{numbers[0], numbers[1]},
                                                                  Point{numbers[2], numbers[3]});
                                                }) 
                                | ranges::to<std::vector<Sensor>>();

    return sensors;
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

    std::cout << "Part 1" << std::endl;
    for (auto &y: std::vector<int>{10, 2000000}) {
        auto ranges = inputs
                    | rv::transform([y](const auto &sensor) { return sensor.exclusion_range(y); })
                    | rv::filter([](const std::optional<range> &optr) { return optr.has_value(); })
                    | rv::transform([](const std::optional<range> &optr) { return optr.value(); })
                    | rv::filter([](const range &r) { return !r.empty(); })
                    | ranges::to<std::vector<range>>();

        auto merged_ranges = merge_ranges(ranges);
        auto total_excluded = std::accumulate(merged_ranges[y].begin(), merged_ranges[y].end(), 0, [](int total, const range &r) { return total + r.size(); });
        std::cout << "y=" << y << ": " << total_excluded << std::endl;
    }

    std::cout << "Part 2" << std::endl;
    for (auto &xy: std::vector<int>{20, 4000000}) {
        auto all_ranges = inputs
                        | rv::transform([](const auto &sensor) { return sensor.exclusion_ranges(false); })
                        | rv::join
                        | rv::filter([](const range &r) { return !r.empty(); })
                        | rv::filter([xy](const range &r) { return r.y >= 0 && r.y <= xy; })
                        | rv::filter([xy](const range &r) { range sup(r.y, 0, xy); return sup.overlaps(r); })
                        | ranges::to<std::vector<range>>();

        auto merged_ranges = merge_ranges(all_ranges);

        auto gaps = merged_ranges
                  | rv::transform([xy](const auto &p) { 
                                      const auto &[y, v] = p;
                                      range entire(y, 0, xy);

                                      std::vector<range> rv;
                                      for (auto &r : v) {
                                          auto g = entire - r;
                                          rv.insert(rv.end(), g.begin(), g.end());
                                      }
                                      return intersection(rv);
                                    })
                  | rv::filter([](const range &r) { return !r.empty(); })
                  | ranges::to<std::vector<range>>();

        auto score = std::accumulate(gaps.begin(), gaps.end(), 0L, 
                                     [](long total, const range &r) { 
                                        long increment = 0;
                                        if (r.start == r.end) {
                                            increment = 4000000L*r.start + r.y;
                                        }
                                        return total + increment; });

        std::cout << "xy = " << xy << " " << score << std::endl;
    }

    return 0;
}