#include "Briscola.h"
#include <stdexcept>
#include <algorithm>

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
            throw std::invalid_argument("Numero carta non valido: " + std::to_string(number));
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
    : briscolaSeed(suit), 
    players(players), 
    nextFirstPlayer(0) {
    //TO DO: resize o assign?? perchè uno mantiene i punteggi per tutta la partita, non solo per il round corrente
    scores.resize(players, 0);

    //
    if(players != 2) {
        throw std::invalid_argument("Number of players must be between 2");
    }
    currentRoundCards.reserve(players);
}

//--------------PUBLIC METHODS--------------

bool Briscola::addCardToRound(Suit suit, int number) {
    cardRank(number); // validate card number, will throw if invalid
 
    if (isRoundComplete())
        throw std::logic_error("Il round è già completo. Chiamare computeRound() prima di aggiungere nuove carte.");
 
    currentRoundCards.push_back({suit, number});
 
    if (isRoundComplete()) {
        computeRound();
        return true;
    }
    return false;
}


std::vector<int> Briscola::getScores() {
    return scores;
}

Suit Briscola::getBriscolaSeed() {
    return briscolaSeed;
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
        throw std::logic_error("Round non ancora completo.");
 
    // La prima carta giocata determina il seme "dominante" (seme di apertura).
    // nextFirstPlayer indica chi ha aperto: la sua carta è in posizione 0,
    // le altre seguono nell'ordine di gioco.
    Suit leadSuit = currentRoundCards[0].first;
 
    // ── Calcolo del vincitore ──────────────────────────────────────────────
    // Regola 1: se una carta è briscola batte qualsiasi carta non-briscola.
    // Regola 2: se entrambe (o nessuna) sono briscola, o entrambe hanno lo
    //           stesso seme dell'apertura, vince quella con rango più alto.
    // Regola 3: se la carta di risposta ha seme diverso dall'apertura
    //           (e nessuna è briscola), vince la carta di apertura.
 
    int winnerLocalIdx = 0;   // indice locale in currentRoundCards
 
    for (int i = 1; i < players; ++i) {
        const std::pair<Suit, int>& challenger = currentRoundCards[i]; //challenger's card
        const std::pair<Suit, int>& current    = currentRoundCards[winnerLocalIdx]; //current winning card
 
        bool challengerIsBriscola = (challenger.first == briscolaSeed);
        bool currentIsBriscola    = (current.first    == briscolaSeed);
 
        if (challengerIsBriscola && !currentIsBriscola) {
            // next player's card is briscola, current winning card is not → next player takes the lead
            winnerLocalIdx = i;
        } else if (!challengerIsBriscola && currentIsBriscola) { // challenger's card is not briscola, current winning card is → current player keeps the lead
            // Il detentore è briscola, lo sfidante no → il detentore mantiene
        } else if (challengerIsBriscola && currentIsBriscola) { //both are briscola, compare rank
            if (cardRank(challenger.second) > cardRank(current.second))
                winnerLocalIdx = i;
        } else { //none is briscola
            if (challenger.first == leadSuit) {
                if (cardRank(challenger.second) > cardRank(current.second)) //challenger has same suit as lead and higher rank → challenger takes the lead
                    winnerLocalIdx = i;
            }
                //challenger has different suit from lead → current winning card keeps the lead
        }
    }
 
    //compute points of the round
    int roundPoints = 0;
    for (const std::pair<Suit, int>& card : currentRoundCards)
        roundPoints += cardPoints(card.second);
 
    // update global scores: the winner of the round is the player who will start the next round, so we update his score
    int globalWinner = (nextFirstPlayer + winnerLocalIdx) % players;
 
    std::vector<int> newScores = scores;
    newScores[globalWinner] += roundPoints;
    setScores(newScores);
 
    // prepare for next round: the winner of this round will start the next round, so we update nextFirstPlayer and clear currentRoundCards
    nextFirstPlayer = globalWinner;
    currentRoundCards.clear();
 
    return globalWinner;
}

