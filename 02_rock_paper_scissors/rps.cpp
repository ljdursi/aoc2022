#include <map>
#include <vector>
#include <numeric>
#include <iostream>
#include <fstream>
#include <sstream>

enum class choice { rock, paper, scissors };

const std::map<choice, choice> beats = {
    {choice::rock, choice::scissors},
    {choice::paper, choice::rock},
    {choice::scissors, choice::paper}
};

const choice lose_choice(const choice a) {
    return beats.at(a);
}

const choice win_choice(const choice a) {
    for (auto [winner, loser] : beats) {
        if (loser == a) {
            return winner;
        }
    }
}

const choice tie(const choice a) {
    return a;
}

const choice opponents_choice(const char a) {
    switch (a) {
        case 'A':
            return choice::rock;
        case 'B':
            return choice::paper;
        case 'C':
            return choice::scissors;
        default:
            throw std::invalid_argument("Invalid choice");
    }
}

struct Strategy {
    virtual choice move(const choice other, const char my_move) const = 0;
};

struct FixedChoiceStrategy: public Strategy {
    const std::map<char, choice> from_encrypted_char_dict = {
        {'X', choice::rock},
        {'Y', choice::paper},
        {'Z', choice::scissors}
    };

    choice move(const choice other, const char my_move) const {
        return from_encrypted_char_dict.at(my_move);
    }
};

struct FixedOutcomeStrategy: public Strategy {
    using choicefunc = std::function<const choice(const choice)>;

    choice move(const choice other, const char my_move) const {
        switch (my_move) {
            case 'X': return lose_choice(other);
            case 'Y': return tie(other);
            case 'Z': return win_choice(other);
        }
    }
};

class RPS_move {
    public:
        const choice state;

    private:
        const std::map<choice, int> play_score = {
            {choice::rock, 1},
            {choice::paper, 2},
            {choice::scissors, 3}
        };

    public:
        RPS_move(choice input) : state(input) {}

        RPS_move(const RPS_move other, const char input, const Strategy *strategy) : state(strategy->move(other.state, input)) {}

        const int score() const {
            return play_score.at(state);
        }

        friend bool operator> (const RPS_move c1, const RPS_move c2) {
            return win_choice(c2.state) == c1.state;
        }

        friend bool operator< (const RPS_move c1, const RPS_move c2) {
            return win_choice(c1.state) == c2.state;
        }

        friend bool operator== (RPS_move c1, const RPS_move c2) {
            return tie(c2.state) == c1.state;
        }

        operator std::string() const {
            switch (state) {
                case choice::rock:
                    return "Rock";
                case choice::paper:
                    return "Paper";
                case choice::scissors:
                    return "Scissors";
            }
        }
};

class RPS_round {
    private:
        RPS_move player1;
        RPS_move player2;

    public:
        RPS_round(const char p1, const char p2, const Strategy *strategy) : 
            player1(opponents_choice(p1)), player2(player1, p2, strategy) {}

        const int win_bonus = 6;
        const int tie_bonus = 3;

        bool player1_wins() const {
            return player1 > player2;
        }

        bool player2_wins() const {
            return player2 > player1;
        }

        bool tie() const {
            return player1 == player2;
        }

        int player1_score() const {
            return player1.score() + (player1_wins() ? win_bonus : 0) + (tie() ? tie_bonus : 0);
        }

        int player2_score() const {
            return player2.score() + (player2_wins() ? win_bonus : 0) + (tie() ? tie_bonus : 0);
        }

        operator std::string() const {
            std::stringstream ss;
            std::pair<int, int> scores = {player1_score(), player2_score()};
            ss << std::string(player1) << " (" << player1_score() << "), ";
            ss << std::string(player2) << " (" << player2_score() << ")";

            return ss.str();
        }

};

std::vector<std::pair<char, char>> get_inputs(std::ifstream &input) {
    // read a list of list of integers from the input file,
    // with each list separated by a blank line

    std::vector<std::pair<char, char>> results;
    std::string line;

    while (std::getline(input, line)) {
        std::istringstream iss(line);
        std::vector<std::string> words{std::istream_iterator<std::string>{iss},
                                       std::istream_iterator<std::string>{}};

        if (words.size() != 2) {
            throw std::invalid_argument("Invalid input");
        }

        results.push_back(std::pair<char, char>(words[0][0], words[1][0]));
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

    auto plays = get_inputs(input);
    std::vector<RPS_round> rounds;

    std::cout << "Part 1" << std::endl;
    FixedChoiceStrategy strategy1;
    std::transform(plays.begin(), plays.end(), std::back_inserter(rounds),
                   [&strategy1](auto play) { return RPS_round(play.first, play.second, &strategy1); });

    auto p1_score = std::accumulate(rounds.begin(), rounds.end(), 0,
                                    [](int acc, auto round) { return acc + round.player2_score(); });

    std::cout << p1_score << std::endl;

    std::cout << "Part 2" << std::endl;
    rounds.clear();
    FixedOutcomeStrategy strategy2;
    std::transform(plays.begin(), plays.end(), std::back_inserter(rounds),
                   [&strategy2](auto play) { return RPS_round(play.first, play.second, &strategy2); });

    auto p2_score = std::accumulate(rounds.begin(), rounds.end(), 0,
                                    [](int acc, auto round) { return acc + round.player2_score(); });

    std::cout << p2_score << std::endl;

    return 0;
}