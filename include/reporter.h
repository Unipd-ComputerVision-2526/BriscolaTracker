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

/**
 * @brief Classe dedicata alla generazione dei report e al calcolo delle metriche.
 */
class Reporter {
public:
    void logRound(const RoundData& data);
    void exportCSV(const std::string& filename) const;
    void generateFinalReport(const std::string& filename, int totalNorth, int totalSouth) const;
    void calculateMetrics(const std::string& groundTruthPath) const;

    void clear();

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
