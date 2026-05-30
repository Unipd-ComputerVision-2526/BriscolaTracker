#include <iostream>
#include "Briscola.h"

int main(){

    Briscola game(Suit::CUPS, 2);

     // Simulate a round of the game
    try {
        game.addCardToRound(Suit::CUPS, 1);  // Player 1 plays Ace of Cups
        game.addCardToRound(Suit::SWORDS, 3); // Player 2 plays 3 of Swords - Player 1 should win this round since Ace is higher than 
        // Get and print the scores after the round
        std::vector<int> scores = game.getScores();
        std::cout << "Scores after the round:" << std::endl;
        for (size_t i = 0; i < scores.size(); ++i) {
            std::cout << "Player " << i + 1 << ": " << scores[i] << " points" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}