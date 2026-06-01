#ifndef BRISCOLA_H
#define BRISCOLA_H

#include<vector>
#include "utils.h"


class Briscola{
    public:
        // Constructor that accepts the Briscola card
        Briscola(Suit seme, int players=2);
        // Submits card to the actual round, returns true if the round is complete
        bool addCardToRound(Suit seme, int number);
        // Returns the current score
        std::vector<int> getScores();
        // Returns the Briscola suit
        Suit getBriscolaSeed();

    private:
        // Updates scores.
        void setScores(const std::vector<int>& newScores);
        // Checks if the round is complete
        bool isRoundComplete();
        // Compute the round winner, update the scores and returns the winner.
        int computeRound();

        // Number of players
        int players;
        // First player that plays this round, winner of the previous round.
        int nextFirstPlayer;
        // Briscola's suit
        Suit briscolaSeed;
        // Current round already played cards, with suit and value.
        std::vector<std::pair<Suit, int>> currentRoundCards;
        // Current players scores
        std::vector<int> scores;    
};

#endif



