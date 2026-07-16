/**
 * @file briscola.h
 * @brief Definition of the Briscola game logic engine.
 * @author Caterina Dri
 */

#ifndef BRISCOLA_H
#define BRISCOLA_H

#include <vector>
#include "utils.h"

/**
 * @enum Player
 * @brief Represents the two players in the game.
 */
enum class Player {
    North = 0,
    South = 1
};

/**
 * @struct Card
 * @brief Represents a single playing card.
 */
struct Card {
    Suit suit;   /**< The suit of the card (e.g., Coins, Cups, Swords, Clubs) */
    int number;  /**< The rank/number of the card (1 to 10) */
};

/**
 * @struct RoundResult
 * @brief Contains the outcome of a single played round.
 */
struct RoundResult {
    Player leader; /**< The player who led the round (played the first card) */
    Player winner; /**< The player who won the round */
    int points;    /**< The total points scored in this round */
};

/**
 * @class Briscola
 * @brief Core engine handling the rules, turns, and scoring of a Briscola game.
 */
class Briscola {
public:
    /**
     * @brief Initializes the game with a given briscola suit and number of players.
     * 
     * @param suit The trump suit (Briscola) for the current game.
     * @param players The number of players (defaults to 2).
     */
    Briscola(Suit suit, int players = 2);

    /**
     * @brief Plays a complete round using the cards associated with North and South.
     * 
     * @param northCard The card played by the North player.
     * @param southCard The card played by the South player.
     * @param leader The player who leads this round.
     * @return RoundResult The structured result containing the leader, winner, and points scored.
     */
    RoundResult playRound(const Card& northCard, const Card& southCard, Player leader);

    /**
     * @brief Retrieves the current scores of all players.
     * 
     * @return std::vector<int> A vector containing the players' scores.
     */
    std::vector<int> getScores() const;

    /**
     * @brief Retrieves the suit of the briscola (trump suit) for this game.
     * 
     * @return Suit The trump suit.
     */
    Suit getBriscolaSuit() const;

    /**
     * @brief Retrieves the index of the player who will lead the next round.
     * 
     * @note This value is undefined before the first round is completely played.
     * @return int The index of the next leading player.
     */
    int getNextFirstPlayer() const { return nextFirstPlayer; }

    /**
     * @brief Retrieves the points scored in the most recently completed round.
     * 
     * @return int The points of the last round.
     */
    int getLastRoundPoints() const { return lastRoundPoints; }

private:
    /**
     * @brief Updates the scores of the players with the provided values.
     * 
     * @param newScores A vector containing the updated scores.
     */
    void setScores(const std::vector<int>& newScores);

    /**
     * @brief Checks if the current round is complete.
     * 
     * A round is considered complete when all players have played a card.
     * 
     * @return true if the round is complete, false otherwise.
     */
    bool isRoundComplete() const;

    /**
     * @brief Computes the winner of the round, calculates points, and updates the scores.
     * 
     * @return int The index of the winning player.
     */
    int computeRound();

    /**
     * @brief Converts a numeric player index to the corresponding Player enum value.
     * 
     * @param index The numeric index (0 or 1).
     * @return Player The corresponding Player enum (North or South).
     */
    Player indexToPlayer(int index) const;

    int players;         /**< Number of players in the game (only 2 supported: North, South). */
    int nextFirstPlayer; /**< Index of the player who leads the current or next round. Set explicitly by playRound(). */
    int lastRoundPoints; /**< Points scored in the most recently completed round. */
    Suit briscolaSuit;   /**< The briscola (trump suit) of the current game. */

    /**
     * @brief Cards played in the current round.
     * 
     * The first element is always the card played by the leader.
     */
    std::vector<std::pair<Suit, int>> currentRoundCards;

    /**
     * @brief Current accumulated scores of the players for the entire game.
     */
    std::vector<int> scores;
};

#endif //BRISCOLA_H