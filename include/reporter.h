#ifndef REPORTER_H
#define REPORTER_H

#include <vector>
#include <string>
#include <fstream>
#include "utils.h"

/**
 * @brief Struttura che contiene i dati di un singolo round per l'esportazione.
 */
struct RoundData {
    int round;
    int northNumber; 
    Suit northSuit;
    int southNumber; 
    Suit southSuit;
    int briscolaNumber; 
    Suit briscolaSuit;
    std::string leader; 
    std::string winner; 
    int points;
};

struct SuitMetrics {
    int expected = 0;
    int correctSuit = 0;
    int exactMatch = 0;
    int wrongSuit = 0;
    int incompleteRound = 0;
};

struct GameMetrics {
    SuitMetrics suits[5]; // Index 1-4 correspond to Suit enum (COINS=1, CUPS=2, SWORDS=3, CLUBS=4)
    int correctCards = 0;
    int correctPlayers = 0;
    int correctBriscola = 0;
    int totalEvaluated = 0;
    int expectedCards = 0;
    int expectedBriscola = 20; // Default 20 rounds
    int totalPlayers = 40;

    void add(const GameMetrics& other) {
        for(int i = 1; i <= 4; ++i) {
            suits[i].expected += other.suits[i].expected;
            suits[i].correctSuit += other.suits[i].correctSuit;
            suits[i].exactMatch += other.suits[i].exactMatch;
            suits[i].wrongSuit += other.suits[i].wrongSuit;
            suits[i].incompleteRound += other.suits[i].incompleteRound;
        }
        correctCards += other.correctCards;
        correctPlayers += other.correctPlayers;
        correctBriscola += other.correctBriscola;
        totalEvaluated += other.totalEvaluated;
        expectedCards += other.expectedCards;
        expectedBriscola += other.expectedBriscola;
        totalPlayers += other.totalPlayers;
    }
};

/**
 * @brief Classe dedicata alla generazione dei report e al calcolo delle metriche.
 */
class Reporter {
public:
    void logRound(const RoundData& data);
    void exportCSV(const std::string& filename) const;
    void generateFinalReport(const std::string& filename, int totalNorth, int totalSouth) const;
    GameMetrics calculateMetrics(const std::string& groundTruthPath) const;

private:
    std::vector<RoundData> history_;

    std::string suitToString(Suit s) const;
    Suit stringToSuitInternal(const std::string& s) const;

    /**
     * @brief Template per la validazione dell'apertura degli stream.
     * Centralizza il controllo is_open() per evitare duplicazioni.
     */
    template <typename T>
    bool checkStream(const T& stream, const std::string& filename) const;
};

#endif // REPORTER_H
