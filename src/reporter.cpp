/**
 * @file reporter.cpp
 * @author Giovanni Stefanuto
 */


#include "reporter.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace {
int safeStoi(const std::string& s) {
    if (s.empty()) return 0;
    try { return std::stoi(s); }
    catch (...) { return 0; }
}

std::string formatPct(int count, int total) {
    if (total == 0) return std::string("-");
    double pct = (static_cast<double>(count) / total) * 100.0;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d (%.1f%%)", count, pct);
    return std::string(buf);
}
}

void Reporter::logRound(const RoundData& data) {
    history_.push_back(data);
}

template <typename T>
bool Reporter::checkStream(const T& stream, const std::string& filename) const {
    if (!stream.is_open()) {
        std::cerr << "Errore: Impossibile gestire il file: " << filename << std::endl;
        return false;
    }
    return true;
}

std::string Reporter::suitToString(Suit s) const {
    switch (s) {
        case COINS:  return "coins";
        case CUPS:   return "cups";
        case SWORDS: return "spades";
        case CLUBS:  return "clubs";
        default:     return "unknown";
    }
}

Suit Reporter::stringToSuitInternal(const std::string& s) const {
    if (s == "coins") return COINS;
    if (s == "cups") return CUPS;
    if (s == "spades" || s == "swords") return SWORDS;
    if (s == "clubs") return CLUBS;
    return COINS;
}

void Reporter::exportCSV(const std::string& filename) const {
    std::ofstream file(filename);
    if (!checkStream(file, filename)) return;

    file << "Round,North_Number,North_Suit,South_Number,South_Suit,Briscola_Number,Briscola_Suit,Leader,Winner,Points\n";

    for (const RoundData& r : history_) {
        file << r.round << ","
             << r.northNumber << "," << suitToString(r.northSuit) << ","
             << r.southNumber << "," << suitToString(r.southSuit) << ","
             << r.briscolaNumber << "," << suitToString(r.briscolaSuit) << ","
             << r.leader << "," << r.winner << "," << r.points << "\n";
    }
    file.close();
}

void Reporter::generateFinalReport(const std::string& filename) const {
    std::ofstream file(filename);
    if (!checkStream(file, filename)) return;

    int totalNorth = 0, totalSouth = 0;
    for (const auto& r : history_) {
        if (r.winner == "North") totalNorth += r.points;
        else if (r.winner == "South") totalSouth += r.points;
    }

    file << "====================================================\n";
    file << "           BRISCOLA TRACKER - FINAL REPORT          \n";
    file << "====================================================\n\n";

    file << std::left << std::setw(8)  << "Round" 
         << std::setw(18) << "North Card" 
         << std::setw(18) << "South Card" 
         << std::setw(10) << "Winner" 
         << "Points\n";
    file << std::string(60, '-') << "\n";

    for (const RoundData& r : history_) {
        std::string nCard = std::to_string(r.northNumber) + " " + suitToString(r.northSuit);
        std::string sCard = std::to_string(r.southNumber) + " " + suitToString(r.southSuit);
        file << std::left << std::setw(8)  << r.round 
             << std::setw(18) << nCard 
             << std::setw(18) << sCard 
             << std::setw(10) << r.winner 
             << r.points << "\n";
    }

    file << "\n" << std::string(60, '=') << "\n";
    file << "GAME SUMMARY\n";
    file << "Total Points North: " << totalNorth << "\n";
    file << "Total Points South: " << totalSouth << "\n";
    file << "WINNER: " << (totalNorth > totalSouth ? "NORTH" : (totalSouth > totalNorth ? "SOUTH" : "DRAW")) << "\n";
    file << "====================================================\n";

    file.close();
}

std::vector<RoundData> Reporter::parseGroundTruth(std::ifstream& file) const {
    std::string line;
    std::vector<RoundData> gt;
    std::getline(file, line); // Consume CSV header.

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        gt.push_back(parseGroundTruthLine(line));
    }

    return gt;
}

RoundData Reporter::parseGroundTruthLine(const std::string& line) const {
    std::stringstream ss(line);
    std::string val;
    RoundData r;

    std::getline(ss, val, ','); r.round = safeStoi(val);
    std::getline(ss, val, ','); r.northNumber = safeStoi(val);
    std::getline(ss, val, ','); r.northSuit = stringToSuitInternal(val);
    std::getline(ss, val, ','); r.southNumber = safeStoi(val);
    std::getline(ss, val, ','); r.southSuit = stringToSuitInternal(val);
    std::getline(ss, val, ','); r.briscolaNumber = safeStoi(val);
    std::getline(ss, val, ','); r.briscolaSuit = stringToSuitInternal(val);
    std::getline(ss, val, ','); r.leader = val;
    if (r.leader == "Sud") r.leader = "South";
    std::getline(ss, val, ','); r.winner = val;
    if (r.winner == "Sud") r.winner = "South";
    std::getline(ss, val, ','); r.points = safeStoi(val);

    return r;
}

void Reporter::updateExpectedMetrics(GameMetrics& metrics, const RoundData& gtRound) const {
    metrics.suits[gtRound.northSuit].expected++;
    metrics.suits[gtRound.southSuit].expected++;
    metrics.expectedCards += 2;
    metrics.totalPlayers += 2;
}

void Reporter::evaluateMatchedRound(const RoundData& detected, const RoundData& gtRound, GameMetrics& metrics) const {
    metrics.totalEvaluated++;

    if (detected.northSuit == gtRound.northSuit) {
        metrics.suits[gtRound.northSuit].correctSuit++;
        if (detected.northNumber == gtRound.northNumber) {
            metrics.suits[gtRound.northSuit].exactMatch++;
            metrics.correctCards++;
        }
    } else {
        metrics.suits[gtRound.northSuit].wrongSuit++;
    }

    if (detected.southSuit == gtRound.southSuit) {
        metrics.suits[gtRound.southSuit].correctSuit++;
        if (detected.southNumber == gtRound.southNumber) {
            metrics.suits[gtRound.southSuit].exactMatch++;
            metrics.correctCards++;
        }
    } else {
        metrics.suits[gtRound.southSuit].wrongSuit++;
    }

    if (detected.leader == gtRound.leader) metrics.correctPlayers++;
    if (detected.winner == gtRound.winner) metrics.correctPlayers++;
}

void Reporter::markIncompleteRound(const RoundData& gtRound, GameMetrics& metrics) const {
    metrics.suits[gtRound.northSuit].incompleteRound++;
    metrics.suits[gtRound.southSuit].incompleteRound++;
}

void Reporter::printMetricsReport(const GameMetrics& metrics, bool showDetailedStats) const {
    std::cout << "\n--- PERFORMANCE METRICS ---" << std::endl;
    std::cout << "Card Recognition Accuracy: " << (metrics.expectedCards > 0 ? (static_cast<double>(metrics.correctCards) / metrics.expectedCards) * 100.0 : 0.0) << "%" << std::endl;
    std::cout << "Player Identification Accuracy: " << (metrics.totalPlayers > 0 ? (static_cast<double>(metrics.correctPlayers) / metrics.totalPlayers) * 100.0 : 0.0) << "%" << std::endl;
    std::cout << "Briscola Recognition Accuracy: " << (metrics.expectedBriscola > 0 ? (static_cast<double>(metrics.correctBriscola) / metrics.expectedBriscola) * 100.0 : 0.0) << "%" << std::endl;
    std::cout << "Game Result Accuracy: " << (metrics.expectedResultFields > 0 ? (static_cast<double>(metrics.correctResultFields) / metrics.expectedResultFields) * 100.0 : 0.0) << "%" << std::endl;
    std::cout << "Rounds Evaluated: " << metrics.totalEvaluated << " / " << (metrics.expectedCards / 2) << std::endl;

    if (showDetailedStats) {
        std::cout << "\n--- DETAILED SUIT METRICS ---\n";
        std::cout << std::left << std::setw(10) << "SUIT"
                  << std::setw(15) << "totale atteso"
                  << std::setw(20) << "Seme corretto"
                  << std::setw(25) << "Seme + Numero esatti"
                  << std::setw(20) << "Seme errato"
                  << std::setw(20) << "Round incompleto"
                  << std::endl;

        std::string suitNames[] = {"", "COINS", "CUPS", "SWORDS", "CLUBS"};
        for (int i = 1; i <= 4; ++i) {
            const auto& sm = metrics.suits[i];
            std::cout << std::left << std::setw(10) << suitNames[i]
                      << std::setw(15) << sm.expected
                      << std::setw(20) << formatPct(sm.correctSuit, sm.expected)
                      << std::setw(25) << formatPct(sm.exactMatch, sm.expected)
                      << std::setw(20) << formatPct(sm.wrongSuit, sm.expected)
                      << std::setw(20) << formatPct(sm.incompleteRound, sm.expected)
                      << std::endl;
        }
    }
}

GameMetrics Reporter::calculateMetrics(const std::string& groundTruthPath, bool showDetailedStats) const {
    GameMetrics metrics;

    std::ifstream file(groundTruthPath);
    if (!checkStream(file, groundTruthPath)) {
        return metrics;
    }

    std::vector<RoundData> gt = parseGroundTruth(file);

    metrics.expectedBriscola = 1;

    for (const auto& gtRound : gt) {
        updateExpectedMetrics(metrics, gtRound);

        auto it = std::find_if(history_.begin(), history_.end(), [&gtRound](const RoundData& h) {
            return h.round == gtRound.round;
        });

        if (it != history_.end()) {
            evaluateMatchedRound(*it, gtRound, metrics);
        } else {
            markIncompleteRound(gtRound, metrics);
        }
    }

    // --- BRISCOLA RECOGNITION ACCURACY ---
    if (!history_.empty() && !gt.empty()) {
        if (history_[0].briscolaNumber == gt[0].briscolaNumber && history_[0].briscolaSuit == gt[0].briscolaSuit) {
            metrics.correctBriscola = 1;
        } else {
            metrics.correctBriscola = 0;
        }
    } else {
        metrics.correctBriscola = 0;
    }

    // --- GAME RESULT ACCURACY ---
    int gtNorthPts = 0, gtSouthPts = 0;
    for (const auto& r : gt) {
        if (r.winner == "North") gtNorthPts += r.points;
        else if (r.winner == "South") gtSouthPts += r.points;
    }
    std::string gtWinner = gtNorthPts > gtSouthPts ? "North" : (gtSouthPts > gtNorthPts ? "South" : "Draw");

    int detNorthPts = 0, detSouthPts = 0;
    for (const auto& r : history_) {
        if (r.winner == "North") detNorthPts += r.points;
        else if (r.winner == "South") detSouthPts += r.points;
    }
    std::string detWinner = detNorthPts > detSouthPts ? "North" : (detSouthPts > detNorthPts ? "South" : "Draw");

    metrics.correctResultFields = 0;
    metrics.expectedResultFields = 3;
    if (detNorthPts == gtNorthPts) metrics.correctResultFields++;
    if (detSouthPts == gtSouthPts) metrics.correctResultFields++;
    if (detWinner == gtWinner) metrics.correctResultFields++;

    printMetricsReport(metrics, showDetailedStats);

    return metrics;
}

// ============================================================================
// CONSOLE OUTPUT METHODS
// ============================================================================

void Reporter::printGameStart(const std::string& gameName) const {
    std::cout << "\n========================================" << std::endl;
    std::cout << " STARTING ANALYSIS: " << gameName << std::endl;
    std::cout << "========================================" << std::endl;
}

void Reporter::printGameEnd(const std::string& gameName) const {
    std::cout << "\n>>> END OF ANALYSIS for " << gameName << ". Final Results:" << std::endl;
}

void Reporter::printBriscolaIdentified(int number, Suit suit, int maxFreq) const {
    std::cout << ">>> Briscola correctly identified: "
              << number << " of " << suitToString(suit)
              << " (seen in " << maxFreq << " frames across first 3 rounds)" << std::endl;
}

void Reporter::printRoundStart(int roundNumber) const {
    std::cout << "\n--- Round " << roundNumber << " ---" << std::endl;
}

void Reporter::printCardRecognized(const std::string& playerName, int number, Suit suit) const {
    std::cout << "Recognized " << playerName << " card: " 
              << number << " of " << suitToString(suit) << std::endl;
}

void Reporter::printRoundFinished(int roundNumber, const std::string& leader, const std::string& winner, int points) const {
    std::cout << "Round " << roundNumber << " FINISHED. Leader: " << leader
              << ". Winner: " << winner << " (" << points << " pts)" << std::endl;
}

void Reporter::printError(const std::string& message) const {
    std::cerr << message << std::endl;
}
