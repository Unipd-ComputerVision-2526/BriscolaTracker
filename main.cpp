/**
 * @file main.cpp
 * @author Caterina Dri
 */

#include <iostream>
#include <vector>
#include <tuple>
#include <string>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <opencv2/opencv.hpp>
#include "utils.h"
#include "eye.h"
#include "gameManager.h"

/**
 * @brief Calculates the percentage of correct identifications vs total attempts.
 */
double calcPct(int correct, int total);

/**
 * @brief Formats a count and its percentage for consistent UI output.
 */
std::string formatStat(int count, int total);

/**
 * @brief Displays command-line usage instructions.
 */
void printHelp();

/**
 * @brief Prints application version and project credits.
 */
void printVersion();

/**
 * @brief Aggregates and prints the final performance metrics for all processed games.
 */
void printGlobalResults(const GameMetrics& totalMetrics, bool showDetailedStats);

// ============================================================================
// MAIN EXECUTION
// ============================================================================

int main(int argc, char* argv[]) {
    bool showDetailedStats = false;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-stat") {
            showDetailedStats = true;
        } else if (arg == "-verbose") {
            verbose = true;
        } else if (arg == "-help") {
            printHelp();
            return 0;
        } else if (arg == "-version") {
            printVersion();
            return 0;
        }
    }

    const std::string datasetPath = "../dataset/Briscola_Trentine";
    const std::string datasetFolderPath = "../dataset/";
    const std::string resultsFolderPath = "../results/";

    std::vector<std::tuple<cv::Mat, Suit, int>> dataset = loadDataset(datasetPath);
    if (dataset.empty()) {
        std::cerr << "Error: Dataset not found in " << datasetPath << std::endl;
        return -1;
    }

    if (verbose) {
        std::cout << "[VERBOSE] Dataset loaded: " << dataset.size() << " total cards for template matching." << std::endl;
    } else {
        std::cout << "Dataset loaded: " << dataset.size() << " cards." << std::endl;
    }

    Eye watcher;
    watcher.fit(dataset);

    GameMetrics totalMetrics;
    GameManager manager(&watcher);

    int g = 1;
    while (true) {
        std::string gameName = "game" + std::to_string(g);
        std::string expectedPath = datasetFolderPath + gameName;

        if (!std::filesystem::exists(expectedPath) || !std::filesystem::is_directory(expectedPath)) {
            break; 
        }

        GameMetrics gm = manager.processFullGame(gameName, datasetFolderPath, resultsFolderPath,showDetailedStats);
        totalMetrics.add(gm);
        
        g++; 
    }
    
    printGlobalResults(totalMetrics, showDetailedStats);
    
    std::cout << "\n>>> ALL THE GAMES HAVE BEEN PROCESSED SUCCESSFULLY!" << std::endl;

    return 0; 
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

double calcPct(int correct, int total) {
    if (total == 0) return 0.0;
    return (static_cast<double>(correct) / total) * 100.0;
}

std::string formatStat(int count, int total) {
    if (total == 0) return "-";
    
    std::stringstream ss;
    ss << count << " (" << std::fixed << std::setprecision(1) << calcPct(count, total) << "%)";
    return ss.str();
}

void printHelp() {
    std::cout << "Usage: ./BriscolaTracker [OPTIONS]\n";
    std::cout << "Options:\n";
    std::cout << "  -stat       Prints the detailed table of suit metrics.\n";
    std::cout << "  -verbose    Shows additional logs and details during processing.\n";
    std::cout << "  -version    Prints the version and the project logo.\n";
    std::cout << "  -help       Shows this help message.\n";
}

void printVersion() {
    std::cout << R"(
  ____       _               _      _______             _             
 |  _ \     (_)             | |    |__   __|           | |            
 | |_) |_ __ _ ___  ___ ___ | | __ _  | |_ __ __ _  ___| | _____ _ __ 
 |  _ <| '__| / __|/ __/ _ \| |/ _` | | | '__/ _` |/ __| |/ / _ \ '__|
 | |_) | |  | \__ \ (_| (_) | | (_| | | | | | (_| | (__|   <  __/ |   
 |____/|_|  |_|___/\___\___/|_|\__,_| |_|_|  \__,_|\___|_|\_\___|_|   
                                                                      
)" << '\n';
    std::cout << "BriscolaTracker v1.0\n";
    std::cout << "Computer Vision Project - University of Padua\n";
    std::cout << "Automatic card and score recognition in a game of Briscola.\n";
}

void printGlobalResults(const GameMetrics& totalMetrics, bool showDetailedStats) {
    std::cout << "\n========================================" << std::endl;
    std::cout << " GLOBAL RESULTS (ALL GAMES)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Card Recognition Accuracy: "   << calcPct(totalMetrics.correctCards, totalMetrics.expectedCards) << "%\n";
    std::cout << "Player Identification Accuracy: " << calcPct(totalMetrics.correctPlayers, totalMetrics.totalPlayers) << "%\n";
    std::cout << "Briscola Recognition Accuracy: " << calcPct(totalMetrics.correctBriscola, totalMetrics.expectedBriscola) << "%\n";
    std::cout << "Game Result Accuracy: "        << calcPct(totalMetrics.correctResultFields, totalMetrics.expectedResultFields) << "%\n";
    
    int expectedRounds = totalMetrics.expectedCards / 2;
    std::cout << "Rounds Evaluated: " << totalMetrics.totalEvaluated << " / " << expectedRounds << "\n";

    if (showDetailedStats) {
        std::cout << "\n--- DETAILED SUIT METRICS (GLOBAL) ---\n";
        
        std::cout << std::left 
                  << std::setw(10) << "SUIT" 
                  << std::setw(15) << "Expected Total"
                  << std::setw(20) << "Correct Suit"
                  << std::setw(25) << "Exact Match (Suit+Num)"
                  << std::setw(20) << "Wrong Suit"
                  << std::setw(20) << "Incomplete Round"
                  << "\n";

        std::string suitNames[] = {"", "COINS", "CUPS", "SWORDS", "CLUBS"};
        
        for (int i = 1; i <= 4; ++i) {
            const auto& sm = totalMetrics.suits[i];
            
            std::cout << std::left 
                      << std::setw(10) << suitNames[i]
                      << std::setw(15) << sm.expected
                      << std::setw(20) << formatStat(sm.correctSuit, sm.expected)
                      << std::setw(25) << formatStat(sm.exactMatch, sm.expected)
                      << std::setw(20) << formatStat(sm.wrongSuit, sm.expected)
                      << std::setw(20) << formatStat(sm.incompleteRound, sm.expected)
                      << "\n";
        }
    }
}