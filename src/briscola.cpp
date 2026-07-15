/**
 * @file briscola.cpp
 * @brief Implementation of the class Briscola game logic engine.
 * @author Caterina Dri
 */

#include "briscola.h"
#include <stdexcept>
#include <string>

// Returns the rank of the card for comparison purposes (higher = stronger card).
// Ranking: 1 > 3 > 10 > 9 > 8 > 7 > 6 > 5 > 4 > 2
static int cardRank(int number) {
    switch (number) {
        case 1:  return 10;
        case 3:  return 9;
        case 10: return 8;
        case 9:  return 7;
        case 8:  return 6;
        case 7:  return 5;
        case 6:  return 4;
        case 5:  return 3;
        case 4:  return 2;
        case 2:  return 1;
        default:
            throw std::invalid_argument("Card number not valid: " + std::to_string(number));
    }
}

// Returns the points of the card.
static int cardPoints(int number) {
    switch (number) {
        case 1:  return 11;
        case 3:  return 10;
        case 10: return 4;
        case 9:  return 3;
        case 8:  return 2;
        default: return 0;   // others: 2,4,5,6,7
    }
}

// ============================================================================
// CONSTRUCTOR
// ============================================================================

Briscola::Briscola(Suit suit, int players)
    : players(players),
      nextFirstPlayer(0), // Overwritten by the leader passed to the first playRound() call
      lastRoundPoints(0),
      briscolaSuit(suit) {

    if (players != 2) {
        throw std::invalid_argument("Briscola only supports 2 players (North/South).");
    }

    scores.resize(players, 0);
    currentRoundCards.reserve(players);
}

// ============================================================================
// PUBLIC METHODS
// ============================================================================
RoundResult Briscola::playRound(const Card& northCard, const Card& southCard, Player leader) {
    cardRank(northCard.number); 
    cardRank(southCard.number); 

    if (isRoundComplete())
        throw std::logic_error("Cannot play round: previous round was not consumed");

    // We push the cards in play order according to the externally provided leader.
    // This ensures currentRoundCards[0] is always the card played by the leader.
    if (leader == Player::North) {
        currentRoundCards.push_back({northCard.suit, northCard.number});
        currentRoundCards.push_back({southCard.suit, southCard.number});
    } else {
        currentRoundCards.push_back({southCard.suit, southCard.number});
        currentRoundCards.push_back({northCard.suit, northCard.number});
    }

    nextFirstPlayer = static_cast<int>(leader);

    int winnerIndex = computeRound(); 

    return RoundResult{
        leader,
        indexToPlayer(winnerIndex),
        lastRoundPoints
    };
}

std::vector<int> Briscola::getScores() const {
    return scores;
}

Suit Briscola::getBriscolaSuit() const {
    return briscolaSuit;
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

void Briscola::setScores(const std::vector<int>& newScores) {
    if (static_cast<int>(newScores.size()) != players)
        throw std::invalid_argument("Dimension of newScores must match number of players");
    scores = newScores;
}

bool Briscola::isRoundComplete() const {
    int playedCards = static_cast<int>(currentRoundCards.size());
    return (playedCards == players) ? true : false;
}

int Briscola::computeRound() {
    if (!isRoundComplete())
        throw std::logic_error("Round is not complete.");

    std::vector<int> playOrder(players);
    for (int i = 0; i < players; ++i)
        playOrder[i] = (nextFirstPlayer + i) % players;

    Suit leadSuit = currentRoundCards[0].first;

    // Briscola rules for determining the winner:
    // Rule 1: briscola beats any non-briscola card.
    // Rule 2: if both are briscola, the higher rank wins.
    // Rule 3: if neither is briscola, the challenger can only win if it
    //         matches the lead suit and has a higher rank.
    int globalWinner = playOrder[0];
    int winnerCardIdx = 0; // index into currentRoundCards of the current winning card

    for (int i = 1; i < players; ++i) {
        const std::pair<Suit, int>& challenger = currentRoundCards[i];
        const std::pair<Suit, int>& current    = currentRoundCards[winnerCardIdx];

        bool challengerIsBriscola = (challenger.first == briscolaSuit);
        bool currentIsBriscola    = (current.first    == briscolaSuit);

        if (challengerIsBriscola && !currentIsBriscola) {
            globalWinner  = playOrder[i];
            winnerCardIdx = i;
        } else if (!challengerIsBriscola && currentIsBriscola) {
            // The current winning card keeps the lead
        } else if (challengerIsBriscola && currentIsBriscola) {
            if (cardRank(challenger.second) > cardRank(current.second)) {
                globalWinner  = playOrder[i];
                winnerCardIdx = i;
            }
        } else { // Neither is briscola
            if (challenger.first == leadSuit &&
                cardRank(challenger.second) > cardRank(current.second)) {
                globalWinner  = playOrder[i];
                winnerCardIdx = i;
            }
        }
    }

    int roundPoints = 0;
    for (const std::pair<Suit, int>& card : currentRoundCards)
        roundPoints += cardPoints(card.second);

    lastRoundPoints = roundPoints;

    std::vector<int> newScores = scores;
    newScores[globalWinner] += roundPoints;
    setScores(newScores);

    currentRoundCards.clear();

    return globalWinner;
}

Player Briscola::indexToPlayer(int index) const {
    if (index == static_cast<int>(Player::North)) {
        return Player::North;
    }
    if (index == static_cast<int>(Player::South)) {
        return Player::South;
    }
    throw std::invalid_argument("Invalid player index.");
}