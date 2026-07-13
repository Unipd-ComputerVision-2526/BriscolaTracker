#ifndef BRISCOLA_H
#define BRISCOLA_H
#include <vector>
#include "utils.h"

// Represents the two players: North = 0, South = 1.
enum class Player {
    North = 0,
    South = 1
};

// Represents a single card with its suit and rank number
struct Card {
    Suit suit;
    int number;
};

// Contains the result of a single round
struct RoundResult {
    Player leader;
    Player winner;
    int points;
};

class Briscola {
public:
    // Initializes the game with a given briscola suit and number of players (default 2)
    Briscola(Suit suit, int players = 2);

    // Plays a complete round using the cards already associated with North and South,
    // and specifies the player who led this round
    RoundResult playRound(const Card& northCard, const Card& southCard, Player leader);

    // Returns the current scores of the players
    std::vector<int> getScores() const;

    // Returns the suit of the briscola (trump suit).
    Suit getBriscolaSuit() const;

    // Returns the index of the player who will lead the next round.
    // Note: Undefined before the first round is played
    int getNextFirstPlayer() const { return nextFirstPlayer; }

    // Returns the points of the last completed round
    int getLastRoundPoints() const { return lastRoundPoints; }

private:
    // Updates the scores of the players with the provided values
    void setScores(const std::vector<int>& newScores);

    // Checks if the current round is complete (i.e., all players have played a card)
    bool isRoundComplete() const;

    // Computes the winner of the round, calculates points, and updates the scores
    int computeRound();

    // Converts a player index (0/1) to the corresponding Player enum value
    Player indexToPlayer(int index) const;

    // Number of players in the game (only 2 supported: North, South)
    int players;

    // Index of the player who leads the current or next round. 
    // Set explicitly by playRound() each time
    int nextFirstPlayer;

    // Points scored in the most recently completed round
    int lastRoundPoints;

    // The briscola (trump suit) of the current game 
    Suit briscolaSuit;

    // Cards played in the current round (suit and rank).
    // The first element is always the card played by the leader.
    std::vector<std::pair<Suit, int>> currentRoundCards;

    // Current scores of the players, for the entire game.
    std::vector<int> scores;
};
#endif //BRISCOLA_H