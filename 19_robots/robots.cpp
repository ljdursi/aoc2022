#include <array>
#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <regex>
#include <execution>
#include <algorithm>
#include <numeric>
#include <map>
#include <queue>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

enum struct material : size_t { ore=0, clay=1, obsidian=2, geode=3, last };

const std::map<std::string, material> valid_materials = {
    {"ore", material::ore},
    {"clay", material::clay},
    {"obsidian", material::obsidian},
    {"geode", material::geode},
};

const std::map<material, std::string> material_names = {
    {material::ore, "ore"},
    {material::clay, "clay"},
    {material::obsidian, "obsidian"},
    {material::geode, "geode"},
    {material::last, "sentinal"},
};

struct Inventory {
    std::array<int, (size_t)material::last> items;    

    void add(material item, int count=1) {
        items[(size_t)item] += count;
    }

    int number_of(material item) const {
        return items.at((size_t)item);
    }

    Inventory& operator+=(const Inventory& other) {
        for (size_t i = 0; i < (size_t)material::last; ++i) {
            items[i] += other.items[i];
        }
        return *this;
    }

    Inventory operator+(const Inventory& other) const {
        Inventory result = *this;
        for (size_t i = 0; i < (size_t)material::last; ++i) {
            result.items[i] += other.items[i];
        }
        return result;
    }

    Inventory& operator-=(const Inventory& other) {
        for (size_t i = 0; i < (size_t)material::last; ++i) {
            items[i] -= other.items[i];
        }
        return *this;
    }

    bool has_enough(const Inventory& other) const {
        for (size_t i = 0; i < (size_t)material::last; ++i) {
            if (items[i] < other.items[i])
                return false;
        }
        return true;
    }

    Inventory() {
        items.fill(0);
    } 

    Inventory(const std::string& recipe) {
        items.fill(0);

        // parse string from "costs..." to period, looking for strings like "3 ore" or "14 clay"
        std::regex re(R"((\d+) ([a-z]+))");
        for (std::sregex_iterator it(recipe.begin(), recipe.end(), re), end; it != end; ++it) {
            std::smatch match = *it;
            std::string item = match[2];
            if (!valid_materials.contains(item)) {
                throw std::invalid_argument("Invalid material: " + item);
            }
            int count = std::stoi(match[1]);
            items[(size_t)valid_materials.at(item)] = count;
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const Inventory& inv) {
        std::vector<std::string> items;

        for (size_t i = 0; i < inv.items.size(); ++i) {
            if (inv.items[i] > 0)
                items.push_back(std::to_string(inv.items[i]) + " " + material_names.at((material)i));
        }

        std::string result = std::accumulate(items.begin(), items.end(), std::string(), [](const std::string& a, const std::string& b) {
            return a + (a.empty() ? "" : " and ") + b;
        });

        os << result;

        return os;
    }
};

struct Blueprint {
    int blueprint_no;
    std::map<material, Inventory> requirements;
    std::map<material, int> max_needed;

    Blueprint() = default;
    Blueprint(const std::string &blueprint) {
        // parse blueprint number from "Blueprint #1:" to colon
        std::regex re(R"(Blueprint (\d+):)");
        std::smatch match;
        if (!std::regex_search(blueprint, match, re)) {
            throw std::invalid_argument("Invalid blueprint: " + blueprint);
        }
        blueprint_no = std::stoi(match[1]);

        // For each sentence of the form "Each <item> robot costs requires <costs>."
        std::regex re2(R"(Each ([a-z]+) robot costs)");
        for (std::sregex_iterator it(blueprint.begin(), blueprint.end(), re2), end; it != end; ++it) {
            std::smatch match = *it;
            std::string item = match[1];
            if (!valid_materials.contains(item)) {
                throw std::invalid_argument("Invalid material: " + item);
            }

            size_t start = match.position();
            std::string recipe = blueprint.substr(start, blueprint.find('.', start) - start);

            requirements[valid_materials.at(item)] = Inventory(recipe);
        }

        for (const material &item : {material::ore, material::clay, material::obsidian, material::geode}) {
            max_needed[item] = 0;
            for (const material &other_item : {material::ore, material::clay, material::obsidian, material::geode}) {
                max_needed[item] = std::max(max_needed[item], requirements[other_item].number_of(item));
            }
        }
        max_needed[material::geode] = 100;
    }

    friend std::ostream& operator<<(std::ostream& os, const Blueprint& bp) {
        os << "Blueprint #" << bp.blueprint_no << ": " << std::endl;
        for (const auto& [item, recipe] : bp.requirements) {
            os << "  Each " << material_names.at(item) << " robot costs " << recipe << "." << std::endl;
        }
        return os;
    }
};

std::vector<Blueprint> get_inputs(std::istream &input) {
    std::vector<std::string> lines;

    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) 
            lines.push_back(line);
    }

    return lines | rv::transform([](const std::string& line) { return Blueprint(line); }) 
                 | ranges::to<std::vector<Blueprint>>();
}

using state = std::tuple<Inventory, Inventory, int>;

struct CustomCompare {
    bool operator()(const state &a, const state &b) {
        auto mat_a = std::get<1>(a);
        auto mat_b = std::get<1>(b);

        if (mat_a.number_of(material::geode) != mat_b.number_of(material::geode))
            return mat_a.number_of(material::geode) < mat_b.number_of(material::geode);

        if (mat_a.number_of(material::obsidian) != mat_b.number_of(material::obsidian))
            return mat_a.number_of(material::obsidian) < mat_b.number_of(material::obsidian);

        return mat_a.number_of(material::clay) < mat_b.number_of(material::clay);
    }
};

void print_pq(auto q) {
    int i = 0;
    for (; !q.empty(); q.pop()) {
        auto [robots, materials, minutes_left] = q.top();
        std::cout << "  " << i++ << ": " << robots << " | " << materials << " " << " | " << minutes_left << std::endl;
    }
}

// I hate these pruning ones.
int bfs_simulate(const Blueprint &bp, const Inventory &robots, const Inventory &materials, const int minutes_remaining) {
    const std::vector<material> items {material::geode, material::obsidian, material::clay, material::ore, material::last};

    std::vector<int> best_so_far(minutes_remaining+1, 0);

    std::priority_queue<state, std::vector<state>, CustomCompare> pq;
    pq.push(std::make_tuple(robots, materials, minutes_remaining));

    while (!pq.empty()) {
        auto [robots, materials, minutes_left] = pq.top();
        pq.pop();

        int ngeodes = materials.items[(size_t)material::geode];
        if (ngeodes < best_so_far[minutes_left] - 2) {
            continue;
        }

        if (ngeodes > best_so_far[minutes_left]) {
            best_so_far[minutes_left] = ngeodes;
        }

        if (minutes_left == 0)
            continue;

        for (const auto &item : items) {
            if (item == material::last) {
                Inventory new_materials = robots + materials;
                pq.push(std::make_tuple(robots, new_materials, minutes_left - 1));
                continue;
            }

            auto recipe = bp.requirements.at(item);
            if (materials.has_enough(recipe)) {
                if (robots.number_of(item) >= bp.max_needed.at(item))
                    continue;
                Inventory new_materials = materials;
                new_materials -= recipe;
                new_materials += robots;

                Inventory new_robots = robots;
                new_robots.add(item, 1);
                pq.push(std::make_tuple(new_robots, new_materials, minutes_left - 1));
            }
        }
    }
    
    return best_so_far[0];
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

    const auto blueprints = get_inputs(input);

    Inventory material_inventory;
    Inventory robots_inventory;
    robots_inventory.add(material::ore, 1);

    auto results = blueprints
                 | rv::transform([&robots_inventory, &material_inventory](const auto& bp) { return std::make_pair(bp.blueprint_no, bfs_simulate(bp, robots_inventory, material_inventory, 24)); })
                 | ranges::to<std::vector<std::pair<int, int>>>();

    for (const auto &[blueprint_no, ngeode] : results) {
        std::cout << "Blueprint #" << blueprint_no << " makes " << ngeode << " geodes." << std::endl;
    }

    int quality_levels = std::accumulate(results.begin(), results.end(), 0, [](int acc, const auto& p) { return acc + p.first*p.second; });

    std::cout << "Part 1" << std::endl;
    std::cout << quality_levels << std::endl;


    std::cout << "Part 2" << std::endl;
    auto results2 = blueprints
                  | rv::take(3)
                  | rv::transform([&robots_inventory, &material_inventory](const auto& bp) { return std::make_pair(bp.blueprint_no, bfs_simulate(bp, robots_inventory, material_inventory, 32)); })
                  | ranges::to<std::vector<std::pair<int, int>>>();

    for (const auto &[blueprint_no, ngeode] : results2) {
        std::cout << "Blueprint #" << blueprint_no << " makes " << ngeode << " geodes." << std::endl;
    }

    std::cout << std::accumulate(results2.begin(), results2.end(), 1, [](int acc, const auto& p) { return acc * p.second; }) << std::endl;

    return 0;
}