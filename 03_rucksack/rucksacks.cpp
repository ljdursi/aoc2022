#include <string>
#include <set>
#include <iostream>
#include <vector>
#include <fstream>
#include <numeric>

class Item {
private:
    char label;
    int priority;

public:
    Item(const char label) : label(label) {
        if (label >= 'a' && label <= 'z')
            priority = label - 'a' + 1;
        else if (label >= 'A' && label <= 'Z')
            priority = label - 'A' + 27;
        else
            priority = 0;
    }

    bool operator== (const Item &other) const {
        return priority == other.priority;
    }
    bool operator< (const Item &other) const {
        return priority < other.priority;
    }
    bool operator> (const Item &other) const {
        return priority > other.priority;
    }

    friend std::ostream& operator<< (std::ostream &out, const Item &item) {
        out << item.label;
        return out;
    }

    const int get_priority() const {
        return priority;
    }
};

using compartment = std::multiset<Item, std::less<Item>>;
using items = std::set<Item, std::less<Item>>;

class Rucksack {
    public:
        const static int n_compartments = 2;

        compartment compartments[n_compartments];

        Rucksack(const std::string &input) {
            const int n = input.length();

            for (int i = 0; i < n_compartments; i++) {
                std::string these_items = input.substr(i*n/2, n / 2);
                for (const char c : these_items) 
                    compartments[i].insert(c);
            }
        }

        friend std::ostream& operator<< (std::ostream &out, const Rucksack &rucksack) {
            out << "|";
            for (int i=0; i<n_compartments; i++) {
                for (const Item &item : rucksack.compartments[i]) out << item;
                out << "|";
            }
            return out;
        }

        Item both_compartments() const {
            compartment intersection;
            for (int i=0; i<n_compartments-1; i++) {
                std::set_intersection(compartments[i].begin(), compartments[i].end(),
                                      compartments[i+1].begin(), compartments[i+1].end(),
                                      std::inserter(intersection, intersection.begin()));
            }

            if (intersection.size() == 0)
                return Item(' ');
            else
                return *intersection.begin();
        }

        items either_compartment() const {
            items union_set;
            for (int i=0; i<n_compartments; i++) {
                union_set.insert(compartments[i].begin(), compartments[i].end());
            }

            return union_set;
        }
};

items common_items(std::vector<Rucksack> &rucksacks) {
    std::vector<items> rucksack_items;
    std::transform(rucksacks.begin(), rucksacks.end(), std::back_inserter(rucksack_items), [](Rucksack &rucksack) { return rucksack.either_compartment(); });

    items common_items(rucksack_items[0]);
    for (int i=1; i<rucksacks.size(); i++) {
        items remaining_items;
        std::set_intersection(common_items.begin(), common_items.end(),
                              rucksack_items[i].begin(), rucksack_items[i].end(),
                              std::inserter(remaining_items, remaining_items.begin()));

        common_items = remaining_items;
    }

    return common_items;
}

std::vector<std::string> get_inputs(std::ifstream &input) {
    // read a list of list of integers from the input file,
    // with each list separated by a blank line

    std::vector<std::string> results;
    std::string line;

    while (std::getline(input, line)) {
        results.push_back(line);
    }

    return results;
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

    std::vector<std::string> inputs = get_inputs(input);

    std::vector<Rucksack> rucksacks;
    std::transform(inputs.begin(), inputs.end(), std::back_inserter(rucksacks),
                   [](const std::string &input) { return Rucksack(input); });

    int part1 = std::accumulate(rucksacks.begin(), rucksacks.end(), 0,
                                [](int sum, const Rucksack &rucksack) {
                                    return sum + rucksack.both_compartments().get_priority();
                                });

    std::cout << "Part 1" << std::endl;
    std::cout << part1 << std::endl;

    std::vector<int> priorities;

    for (int i=0; i<rucksacks.size(); i+=3) {
        std::vector<Rucksack> group(rucksacks.begin()+i, rucksacks.begin()+i+3);
        auto common = common_items(group);

        auto first_item = *(common.begin());
        priorities.push_back(first_item.get_priority());
    }

    int part2 = std::accumulate(priorities.begin(), priorities.end(), 0);

    std::cout << "Part 2" << std::endl;
    std::cout << part2 << std::endl;

    return 0;
}