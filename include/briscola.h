#ifndef BRISCOLA_H
#define BRISCOLA_H

#include<vector>
//TO DO
//#include "utils.h"

//TO DO: TOGLIERLO! c'è già in utils.h
//alcune classi vengono messe con const! controllare questa cosa
enum class Suit {
    COINS,   // Denari
    CUPS,    // Coppe
    SWORDS,  // Spade
    CLUBS    // Bastoni
};

class Briscola{
    public:
        // Costructor that initializes the game with a given briscola seed and number of players (default 2).
        Briscola(Suit suit, int players=2);
        // Add the card played by the current player to the round. Returns true if the round is complete (all players have played), false otherwise.
        bool addCardToRound(Suit suit, int number);
        // Return the current scores of the players.
        std::vector<int> getScores();
        // Return the suit of the briscola.
        Suit getBriscolaSeed();

    private:
        // Update the scores of the players with the new scores provided
        void setScores(const std::vector<int>& newScores);
        // Check if the round is complete (i.e., all players have played their card).
        bool isRoundComplete() const;
        // Compute the winner of the round and update the scores.
        int computeRound();

        // Number of players in the game (default 2, North e South)
        int players;
        // Index of the player who will play first in the next round (0-based index).
        int nextFirstPlayer;
        // Seed of the briscola for the game, which determines the Briscola (trump suit).
        Suit briscolaSeed;
        // Cards played in the current round, with suit and number.
        std::vector<std::pair<Suit, int>> currentRoundCards;
        // Current scores of the players, for the entire game.
        std::vector<int> scores;    
};

#endif



