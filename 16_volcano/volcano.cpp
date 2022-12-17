#include <string>
#include <map>
#include <set>
#include <tuple>
#include <limits>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "range/v3/all.hpp"

namespace rv = ranges::views;

struct room_state {
    bool valve;
    int minute;

    void open_valve(int minute) {
        valve = true;
        this->minute = minute;
    }
    void un_open_valve() {
        valve = false;
        this->minute = -1;
    }
};

class Graph {
    std::map<std::string, std::vector<std::pair<std::string, int>>> graph;

    public:
        Graph(const std::map<std::string, std::vector<std::string>> &tunnels) {
            for (const auto &[room, neighbours] : tunnels) {
                graph[room] = neighbours | rv::transform([&](const auto &room) { return std::make_pair(room, 1); }) | ranges::to<std::vector<std::pair<std::string, int>>>();
            }
        }

        void calc_all_distances() {
            std::vector<std::string> rooms = graph | rv::keys | ranges::to<std::vector<std::string>>();
            std::map<std::string, int> room_to_index;
            const int n = rooms.size();

            std::vector<std::vector<int>> distances(n, std::vector<int>(n, 100000));
            for (int i = 0; i < n; i++) {
                distances[i][i] = 0;
                room_to_index[rooms[i]] = i;
            }

            for (const auto &[room, neighbours] : graph) {
                for (const auto &[neighbour, distance] : neighbours) {
                    distances[room_to_index[room]][room_to_index[neighbour]] = distance;
                }
            }

            for (int k = 0; k < n; k++) {
                for (int i = 0; i < n; i++) {
                    for (int j = 0; j < n; j++) {
                        if ((long)distances[i][j] > (long)(distances[i][k] + distances[k][j])) {
                            distances[i][j] = distances[i][k] + distances[k][j];
                        }
                    }
                }
            }

            graph.clear();
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    if (distances[i][j] != INT_MAX && i != j) {
                        graph[rooms[i]].push_back(std::make_pair(rooms[j], distances[i][j]));
                    }
                }
            }
        }

        void remove_node(std::string node) {
            graph.erase(node);    
            for (auto &[room, neighbours] : graph) {
                neighbours.erase(std::remove_if(neighbours.begin(), neighbours.end(), [&](const auto &p) { return p.first == node; }), neighbours.end());
            }
        }

        std::vector<std::pair<std::string, int>> get_neighbours(std::string node) {
            return graph[node];
        }

        friend std::ostream& operator<<(std::ostream &os, const Graph &g) {
            for (const auto &[room, neighbours] : g.graph) {
                os << room << " -> ";
                for (const auto &[neighbour, distance] : neighbours) {
                    os << neighbour << " (" << distance << ") ";
                }
                os << std::endl;
            }
            return os;
        }
};


class Tunnels {
    std::map<std::string, int> flow_rates;
    std::map<std::string, room_state> room_states;
    std::set<std::string> rooms;
    Graph *g;
    int n_nonzero_valves;

    std::map<std::set<std::string>, int> memo_scores;

    void update_memo_scores(const std::vector<std::string> &path, int score) {
        std::set<std::string> path_set(path.begin(), path.end());
        if (memo_scores.find(path_set) == memo_scores.end()) {
            memo_scores[path_set] = score;
        } else {
            memo_scores[path_set] = std::max(memo_scores[path_set], score);
        }
    }

    std::tuple<std::vector<std::string>, int> DFS(const std::string &start, const int last_minute, std::map<std::string, room_state> local_room_states, std::vector<std::string> path={}, int minute=1) {
        int n_turned_on = ranges::count_if(local_room_states, [](const auto &p) { const auto &[room, state] = p; return state.valve; });

        auto total_flow = [&](const auto &room_states) {
                                   return std::accumulate(room_states.begin(), room_states.end(), 0, 
                                   [&](const auto &sum, const auto &p) {
                                           const auto &[room, state] = p;
                                           int rate = state.valve ? this->flow_rates[room] : 0;
                                           int flow = rate * (last_minute + 1 - state.minute);
                                           return sum + flow; }); };

        auto best_so_far = std::make_tuple(path, total_flow(local_room_states));

        if ((n_turned_on == n_nonzero_valves) || (minute >= last_minute)) {
            update_memo_scores(path, total_flow(local_room_states));
            return best_so_far;
        }

        path.push_back(start);
        if (!local_room_states[start].valve && flow_rates[start] > 0) {
            minute++;
            local_room_states[start].open_valve(minute); 

            if (total_flow(local_room_states) > std::get<1>(best_so_far)) {
                update_memo_scores(path, total_flow(local_room_states));
                best_so_far = std::make_tuple(path, total_flow(local_room_states));
            }
        } 

        for (const auto &[neighbour, distance] : g->get_neighbours(start)) {
            if (local_room_states[neighbour].valve) continue;
            if (flow_rates[neighbour] == 0) continue;
            if (distance + minute > last_minute) continue;

            const auto &[path1, flow1] = DFS(neighbour, last_minute, local_room_states, path, minute+distance);
            update_memo_scores(path1, flow1);

            if (flow1 > std::get<1>(best_so_far)) {
                best_so_far = std::make_tuple(path1, flow1);
            }
        } 

        return best_so_far;
    }

public:
    Tunnels(const std::string start, const std::vector<std::string> &room_names, const std::vector<int> &flow_rates, const std::vector<std::vector<std::string>> &tunnels) {
        std::map<std::string, std::vector<std::string>> tunnel_map;
        for (const auto &[room_name, rate, connecting_tunnels] : rv::zip(room_names, flow_rates, tunnels)) {
            this->rooms.insert(room_name);
            this->flow_rates[room_name] = rate;
            this->room_states[room_name] = room_state{false, 0};
            tunnel_map[room_name] = connecting_tunnels;
        }

        g = new Graph(tunnel_map);

        g->calc_all_distances();

        for (const auto &[room, flow] : rv::zip(room_names,flow_rates)) {
            if (flow == 0 && room != start) {
                g->remove_node(room);
            }
        }

        n_nonzero_valves = ranges::count_if(flow_rates, [](const auto &fr) { return fr > 0; });
    }

    Graph *get_graph() {
        return g;
    }

    std::tuple<std::vector<std::string>, int> get_best_path(std::string start, const int last_minute) {
        return DFS(start, last_minute, room_states);
    }

    std::vector<std::pair<int, std::set<std::string>>> get_best_non_overlapping_paths(std::string start, const int last_minute) {
        memo_scores.clear();
        DFS(start, last_minute, room_states);

        int best_score = 0;
        std::vector<std::pair<int, std::set<std::string>>> result = {};

        for (const auto &[path1, score1] : memo_scores) {
            if (score1 > best_score) 
                result = {{score1, path1}};

            for (const auto &[path2, score2] : memo_scores) {
                std::set<std::string> remaining_rooms;
                std::set_intersection(path1.begin(), path1.end(), path2.begin(), path2.end(), std::inserter(remaining_rooms, remaining_rooms.begin()));
                if (remaining_rooms.size() > 1) continue;

                if (score1 + score2 > best_score) {
                    best_score = score1 + score2;
                    result = {{score1, path1}, {score2, path2}};
                }
            }
        }

        return result;
    }
}; 

std::tuple<std::vector<std::string>, std::vector<int>, std::vector<std::vector<std::string>>>  get_inputs(std::istream &input) {
    std::vector<std::string> lines;

    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) 
            lines.push_back(line);
    }

    auto room_names = lines
                    | rv::transform([](const auto &line) {
                                        std::regex re("^Valve ([A-Z]+)");
                                        std::smatch match;
                                        std::regex_search(line, match, re);
                                        return match[1].str();
                                    })
                    | ranges::to<std::vector<std::string>>();

    auto flow_rates = lines
                    | rv::transform([](const auto &line) {
                                        std::regex re("flow rate=(\\d+)");
                                        std::smatch match;
                                        std::regex_search(line, match, re);
                                        return std::stoi(match[1].str());
                        })
                    | ranges::to<std::vector<int>>();

    auto neighbours = lines
                    | rv::transform([](const auto &line) {
                                        std::string delim = "leading to valves ";
                                        int found = line.find(delim) + delim.size();
                                        std::string ns = line.substr(found);
                                        std::vector<std::string> neighbours;

                                        std::regex re("([A-Z]+)");
                                        std::smatch match;
                                        for (std::sregex_iterator it(ns.begin(), ns.end(), re), end; it != end; ++it) {
                                                neighbours.push_back(it->str());
                                        }
                                        return neighbours;})
                    | ranges::to<std::vector<std::vector<std::string>>>();

    return std::make_tuple(room_names, flow_rates, neighbours);
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

    const auto &[room_names, flow_rates, tunnels] = get_inputs(input);

    Tunnels t("AA", room_names, flow_rates, tunnels);

    auto [path, flow] = t.get_best_path("AA", 30);

    std::cout << "Part 1" << std::endl;
    std::cout << flow << std::endl;

    auto best_score = t.get_best_non_overlapping_paths("AA", 26);

    std::cout << "Part 2" << std::endl;
    std::cout << std::accumulate(best_score.begin(), best_score.end(), 0, [](const auto &acc, const auto &p) { return acc + p.first; }) << std::endl;

    return 0;
}