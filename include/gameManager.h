#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "briscola.h"
#include "eye.h"
#include "reporter.h"

class GameManager {
public:
    // Initializes the game manager with a pre-trained Eye system to avoid reloading datasets
    GameManager(Eye* visionSystem);

    // Destructor
    ~GameManager();

    /**
     * @brief Processes an entire game (20 rounds).
     * @param gameName The identifier of the game (e.g., "game1").
     * @param baseFolderPath The path where the game folder is located.
     * @param showDetailedStats If true, prints detailed suit stats after the game.
     * @return GameMetrics containing the results of the evaluation against ground truth.
     */
    GameMetrics processFullGame(const std::string& gameName, const std::string& baseFolderPath, bool showDetailedStats = false);

private:
    // Pointer to the shared computer vision system (not owned)
    Eye* watcher;

    // Game logic engine for the current game
    std::unique_ptr<Briscola> gameEngine;

    // Handles logging of round results and metrics
    Reporter reporter;

    // True if the static briscola has been identified
    bool isBriscolaIdentified;

    // Stores the briscola card (suit and rank)
    Card currentBriscolaCard;

    // Analyzes the initial frames of the game to identify the static briscola card
    void identifyBriscola(const std::string& gameName, const std::string& baseFolderPath);

    // Processes a single round video, updates the game state, and logs the results
    void playSingleRound(int roundNumber, const std::string& videoPath, const std::string& gameName);

    // Maps a player index (0 or 1) to their string representation ("North" or "South")
    std::string playerIdxToName(int idx) const;
};

#endif // GAMEMANAGER_H