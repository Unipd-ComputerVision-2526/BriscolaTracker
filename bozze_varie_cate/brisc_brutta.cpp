#include "briscola.h" 
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <string>

//returns the rank of the card for comparison purposes (higher = stronger card)
//ranking: 1 > 3 > 10 > 9 > 8 > 7 > 6 > 5 > 4 > 2
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
 
// return the points of the card
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

//--------------CONSTRUCTOR--------------

Briscola::Briscola(Suit suit, int players) 
    : players(players), 
    nextFirstPlayer(0),
    lastRoundPoints(0),
    briscolaSuit(suit) {

    //briscola must have 2 or 4 players
    if (players != 2 && players != 4) { 
        throw std::invalid_argument("Invalid number of players for Briscola."); 
    }

    scores.resize(players, 0);
    currentRoundCards.reserve(players);
}

//--------------PUBLIC METHODS--------------

bool Briscola::addCardToRound(Suit suit, int number) {
    cardRank(number); // validate card number, will throw if invalid
 
    if (isRoundComplete())
        throw std::logic_error("Cannot add the card: round is already complete");
 
    currentRoundCards.push_back({suit, number});
 
    if (isRoundComplete()) {
        computeRound();
        return true;
    }
    return false;
}

//method for 2 players: North and Sud, which cards are already associated
RoundResult Briscola::playRound(const Card& northCard, const Card& southCard) {
    cardRank(northCard.number); // validate card number, will throw if invalid
    cardRank(southCard.number); // validate card number, will throw if invalid

    int leader = nextFirstPlayer;
    int secondPlayer = (leader + 1) % players;

    Card firstCard;
    Card secondCard;

    if (leader == static_cast<int>(Player::North)) {
        firstCard = northCard;
        secondCard = southCard;
    } else {
        firstCard = southCard;
        secondCard = northCard;
    }

    // This method receives the two cards already ordered by play.
    // firstCard  = card played by the leader
    // secondCard = card played by the other player
    int globalWinner = leader;
    bool firstIsBriscola  = (firstCard.suit == briscolaSuit);
    bool secondIsBriscola = (secondCard.suit == briscolaSuit);

    if (firstIsBriscola && !secondIsBriscola) {
        globalWinner = leader;
    } else if (!firstIsBriscola && secondIsBriscola) {
        globalWinner = secondPlayer;
    } else if (firstIsBriscola && secondIsBriscola) {
        if (cardRank(secondCard.number) > cardRank(firstCard.number)) {
            globalWinner = secondPlayer;
        }
    } else {
        if (secondCard.suit != firstCard.suit) {
            globalWinner = leader;
        } else if (cardRank(secondCard.number) > cardRank(firstCard.number)) {
            globalWinner = secondPlayer;
        }
    }

    int roundPoints = cardPoints(northCard.number) + cardPoints(southCard.number);

    lastRoundPoints = roundPoints;
    std::cout<<"POINTS OF THIS ROUND: "<<roundPoints<<std::endl;

    std::vector<int> newScores = scores;
    newScores[globalWinner] += roundPoints;
    setScores(newScores);

    // prepare for next round: the winner of this round will start the next round
    nextFirstPlayer = globalWinner;

    return RoundResult{
        indexToPlayer(leader),
        indexToPlayer(globalWinner),
        roundPoints
    };
}

std::vector<int> Briscola::getScores() const {
    return scores;
}

Suit Briscola::getBriscolaSuit() const {
    return briscolaSuit;
}

//--------------PRIVATE METHODS--------------

void Briscola::setScores(const std::vector<int>& newScores) {
    if (static_cast<int>(newScores.size()) != players)
        throw std::invalid_argument("Dimension of newScores must match number of players");
    scores = newScores;
}

bool Briscola::isRoundComplete() const {
    return static_cast<int>(currentRoundCards.size()) == players;
}

int Briscola::computeRound() {
    if (!isRoundComplete())
        throw std::logic_error("Round is not complete.");

    // Determine the playing order for this round, starting from nextFirstPlayer
    //playOrder[0] = nextFirstPlayer, playOrder[1] = (nextFirstPlayer + 1) % players, etc.
    std::vector<int> playOrder(players);
    for (int i = 0; i < players; ++i)
        playOrder[i] = (nextFirstPlayer + i) % players;
 
    // The first card played determines the "dominant" suit (opening suit).
    // nextFirstPlayer indicates who opened: their card is in position 0,
    // the others follow in playing order.
    Suit leadSuit = currentRoundCards[0].first;
 
    // ── Computing the winner ──────────────────────────────────────────────
    // Rule 1: if a card is briscola it beats any non-briscola card.
    // Rule 2: if both (or neither) are briscola, or both have the
    //           same suit as the opening, the one with the higher rank wins.
    // Rule 3: if the response card has a different suit from the opening
    //           (and neither is briscola), the opening card wins.

    // We assume that the first player (playOrder[0]) is the winner until we find a challenger that beats him.
    int globalWinner = playOrder[0];
    int winnerCardIdx = 0; //index of the winning card in currentRoundCards
 
    for (int i = 1; i < players; ++i) {
        const std::pair<Suit, int>& challenger = currentRoundCards[i]; //challenger's card
        const std::pair<Suit, int>& current    = currentRoundCards[winnerCardIdx]; //current winning card

        bool challengerIsBriscola = (challenger.first == briscolaSuit);
        bool currentIsBriscola    = (current.first    == briscolaSuit);

        if (challengerIsBriscola && !currentIsBriscola) {
            // next player's card is briscola, current winning card is not → next player takes the lead
            globalWinner  = playOrder[i];
            winnerCardIdx = i;
        } else if (!challengerIsBriscola && currentIsBriscola) { // challenger's card is not briscola, current winning card is → current player keeps the lead
            // winnerCardIdx stays the same
        } else if (challengerIsBriscola && currentIsBriscola) { //both are briscola, compare rank
            if (cardRank(challenger.second) > cardRank(current.second)) {
                globalWinner  = playOrder[i];
                winnerCardIdx = i;
            }
        } else { //none is briscola
            if (challenger.first == leadSuit) {
                if (cardRank(challenger.second) > cardRank(current.second)) { //challenger has same suit as lead and higher rank → challenger takes the lead
                    globalWinner  = playOrder[i];
                    winnerCardIdx = i;
                }
            }
            //challenger has different suit from lead → current winning card keeps the lead
        }
    }
 
    //compute points of the round
    int roundPoints = 0;
    for (const std::pair<Suit, int>& card : currentRoundCards)
        roundPoints += cardPoints(card.second);
 
    lastRoundPoints = roundPoints;
    std::cout<<"POINTS OF THIS ROUND: "<<roundPoints<<std::endl;
    std::vector<int> newScores = scores;
    newScores[globalWinner] += roundPoints;
    setScores(newScores);
 
    // prepare for next round: the winner of this round will start the next round, so we update nextFirstPlayer and clear currentRoundCards
    nextFirstPlayer = globalWinner;
    currentRoundCards.clear();
 
    return globalWinner;
}

Player Briscola::indexToPlayer(int index) const {
    if (index == 0) {
        return Player::North;
    }

    if (index == 1) {
        return Player::South;
    }

    throw std::invalid_argument("Invalid player index.");
}