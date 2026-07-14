#include "reporter.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>

void Reporter::clear() {
    history_.clear();
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

void Reporter::generateFinalReport(const std::string& filename, int totalNorth, int totalSouth) const {
    std::ofstream file(filename);
    if (!checkStream(file, filename)) return;

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

void Reporter::calculateMetrics(const std::string& groundTruthPath) const {
    std::ifstream file(groundTruthPath);
    if (!checkStream(file, groundTruthPath)) return;

    auto safeStoi = [](const std::string& s) {
        if (s.empty()) return 0;
        try { return std::stoi(s); }
        catch (...) { return 0; }
    };

    std::string line, val;
    std::vector<RoundData> gt;
    std::getline(file, line); // Skip header

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        RoundData r;
        std::getline(ss, val, ','); r.round = safeStoi(val); // Read Round column
        std::getline(ss, val, ','); r.northNumber = safeStoi(val);
        std::getline(ss, val, ','); r.northSuit = stringToSuitInternal(val);
        std::getline(ss, val, ','); r.southNumber = safeStoi(val);
        std::getline(ss, val, ','); r.southSuit = stringToSuitInternal(val);
        std::getline(ss, val, ','); r.briscolaNumber = safeStoi(val);
        std::getline(ss, val, ','); r.briscolaSuit = stringToSuitInternal(val);
        std::getline(ss, val, ','); r.leader = val;
        // Fix typo in ground truth ("Sud" instead of "South")
        if (r.leader == "Sud") r.leader = "South";
        std::getline(ss, val, ','); r.winner = val;
        if (r.winner == "Sud") r.winner = "South";
        std::getline(ss, val, ','); r.points = safeStoi(val);
        gt.push_back(r);
    }

    int correctCards = 0, correctPlayers = 0, correctBriscola = 0;
    int totalEvaluated = 0;

    for (const auto& loggedRound : history_) {
        // Find matching round in ground truth
        auto it = std::find_if(gt.begin(), gt.end(), [&loggedRound](const RoundData& g) {
            return g.round == loggedRound.round;
        });

        if (it != gt.end()) {
            totalEvaluated++;
            if (loggedRound.northNumber == it->northNumber && loggedRound.northSuit == it->northSuit) correctCards++;
            if (loggedRound.southNumber == it->southNumber && loggedRound.southSuit == it->southSuit) correctCards++;
            if (loggedRound.leader == it->leader) correctPlayers++;
            if (loggedRound.winner == it->winner) correctPlayers++;
            if (loggedRound.briscolaNumber == it->briscolaNumber && loggedRound.briscolaSuit == it->briscolaSuit) correctBriscola++;
        }
    }

    std::cout << "\n--- PERFORMANCE METRICS ---" << std::endl;
    // We expect exactly 40 cards across 20 rounds, and 40 player ID validations.
    // If some rounds are missing, they count as wrong (hence dividing by 40.0)
    std::cout << "Card Recognition Accuracy: " << (static_cast<double>(correctCards) / 40.0) * 100.0 << "%" << std::endl;
    std::cout << "Player Identification Accuracy: " << (static_cast<double>(correctPlayers) / 40.0) * 100.0 << "%" << std::endl;
    std::cout << "Briscola Recognition Accuracy: " << (static_cast<double>(correctBriscola) / 20.0) * 100.0 << "%" << std::endl;
    std::cout << "Rounds Evaluated: " << totalEvaluated << " / 20" << std::endl;
}
