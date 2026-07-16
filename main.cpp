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
#include <opencv2/opencv.hpp>
#include "utils.h"
#include "eye.h"
#include "gameManager.h"

int main(int argc, char* argv[]) {
    bool showDetailedStats = false;
    bool verbose = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-stat") {
            showDetailedStats = true;
        } else if (std::string(argv[i]) == "-verbose") {
            verbose = true;
        } else if (std::string(argv[i]) == "-help") {
            std::cout << "Usage: ./BriscolaTracker [OPTIONS]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -stat       Prints the detailed table of suit metrics." << std::endl;
            std::cout << "  -verbose    Shows additional logs and details during processing." << std::endl;
            std::cout << "  -version    Prints the version and the project logo." << std::endl;
            std::cout << "  -help       Shows this help message." << std::endl;
            return 0;
        } else if (std::string(argv[i]) == "-version") {
            std::cout << R"(
  ____       _               _      _______             _             
 |  _ \     (_)             | |    |__   __|           | |            
 | |_) |_ __ _ ___  ___ ___ | | __ _  | |_ __ __ _  ___| | _____ _ __ 
 |  _ <| '__| / __|/ __/ _ \| |/ _` | | | '__/ _` |/ __| |/ / _ \ '__|
 | |_) | |  | \__ \ (_| (_) | | (_| | | | | | (_| | (__|   <  __/ |   
 |____/|_|  |_|___/\___\___/|_|\__,_| |_|_|  \__,_|\___|_|\_\___|_|   
                                                                      
)" << '\n';
            std::cout << "BriscolaTracker v1.0" << std::endl;
            std::cout << "Computer Vision Project - University of Padua" << std::endl;
            std::cout << "Automatic card and score recognition in a game of Briscola." << std::endl;
            return 0;
        }
    }

    std::string datasetPath = "../dataset/Briscola_Trentine";
    std::string baseFolderPath = "../dataset/";

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

    for (int g = 1; g <= 4; ++g) {
        std::string gameName = "game" + std::to_string(g);
        std::string expectedPath = baseFolderPath + gameName;

        if (!std::filesystem::exists(expectedPath) || !std::filesystem::is_directory(expectedPath)) {
            break; 
        }

        GameMetrics gm = manager.processFullGame(gameName, baseFolderPath, showDetailedStats);
        totalMetrics.add(gm);
    }

    auto formatPct = [](int count, int total) {
        if (total == 0) return std::string("-");
        double pct = (static_cast<double>(count) / total) * 100.0;
        char buf[32];
        snprintf(buf, sizeof(buf), "%d (%.1f%%)", count, pct);
        return std::string(buf);
    };

    std::cout << "\n========================================" << std::endl;
    std::cout << " GLOBAL RESULTS (ALL GAMES)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Card Recognition Accuracy: " << (totalMetrics.expectedCards > 0 ? (static_cast<double>(totalMetrics.correctCards) / totalMetrics.expectedCards) * 100.0 : 0.0) << "%" << std::endl;
    std::cout << "Player Identification Accuracy: " << (totalMetrics.totalPlayers > 0 ? (static_cast<double>(totalMetrics.correctPlayers) / totalMetrics.totalPlayers) * 100.0 : 0.0) << "%" << std::endl;
    std::cout << "Briscola Recognition Accuracy: " << (totalMetrics.expectedBriscola > 0 ? (static_cast<double>(totalMetrics.correctBriscola) / totalMetrics.expectedBriscola) * 100.0 : 0.0) << "%" << std::endl;
    std::cout << "Game Result Accuracy: " << (totalMetrics.expectedResultFields > 0 ? (static_cast<double>(totalMetrics.correctResultFields) / totalMetrics.expectedResultFields) * 100.0 : 0.0) << "%" << std::endl;
    std::cout << "Rounds Evaluated: " << totalMetrics.totalEvaluated << " / " << (totalMetrics.expectedCards / 2) << std::endl;

    if (showDetailedStats) {
        std::cout << "\n--- DETAILED SUIT METRICS (GLOBAL) ---\n";
        std::cout << std::left << std::setw(10) << "SUIT" 
                  << std::setw(15) << "Expected Total"
                  << std::setw(20) << "Correct Suit"
                  << std::setw(25) << "Exact Match (Suit+Num)"
                  << std::setw(20) << "Wrong Suit"
                  << std::setw(20) << "Incomplete Round"
                  << std::endl;

        std::string suitNames[] = {"", "COINS", "CUPS", "SWORDS", "CLUBS"};
        for (int i = 1; i <= 4; ++i) {
            const auto& sm = totalMetrics.suits[i];
            std::cout << std::left << std::setw(10) << suitNames[i]
                      << std::setw(15) << sm.expected
                      << std::setw(20) << formatPct(sm.correctSuit, sm.expected)
                      << std::setw(25) << formatPct(sm.exactMatch, sm.expected)
                      << std::setw(20) << formatPct(sm.wrongSuit, sm.expected)
                      << std::setw(20) << formatPct(sm.incompleteRound, sm.expected)
                      << std::endl;
        }
    }

    std::cout << "\n>>> ALL THE GAMES HAVE BEEN PROCESSED SUCCESSFULLY!" << std::endl;

    return 0;
}