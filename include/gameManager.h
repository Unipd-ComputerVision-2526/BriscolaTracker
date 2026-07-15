/**
 * @file gameManager.h
 * @brief Definition of the GameManager class for managing the game flow and interfacing between the visual module (Eye) and the logic engine (Briscola).
 * @author Caterina Dri
 */

#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "briscola.h"
#include "eye.h"
#include "reporter.h"

/**
 * @class GameManager
 * @brief Orchestrator class that bridges the Computer Vision system and the Briscola game engine.
 * 
 * This class is responsible for loading the video frames, passing them to the Eye module 
 * for card recognition, managing the game state via the Briscola engine, and logging 
 * the results using the Reporter.
 */
class GameManager {
public:
    /**
     * @brief Constructs a new GameManager object.
     * 
     * @param visionSystem Pointer to the Eye module used for card recognition.
     * @throw std::invalid_argument if the visionSystem pointer is null.
     */
    GameManager(Eye* visionSystem);

    /**
     * @brief Destroys the GameManager object.
     */
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
    Eye* watcher;                         ///< Pointer to the shared computer vision system (not owned).
    std::unique_ptr<Briscola> gameEngine; ///< Game logic engine for the current game.
    Reporter reporter;                    ///< Handles logging of round results and performance metrics.
    bool isBriscolaIdentified;            ///< True if the static briscola card has been successfully identified.
    Card currentBriscolaCard;             ///< Stores the identified briscola card (suit and rank).
    bool briscolaPickedUp = false;       ///< True once the static briscola card has been confirmed off the table; stays true for the rest of the game.

    /**
     * @brief Converts a player index to their corresponding name.
     * 
     * @param idx The index of the player (0 for North, 1 for South).
     * @return std::string The name of the player ("North" or "South").
     */
    std::string playerIdxToName(int idx) const;

    /**
     * @brief Constructs the file path for a specific round's video.
     * 
     * @param baseFolderPath The base directory containing the game folders.
     * @param gameName The name of the current game.
     * @param round The round number.
     * @return std::string The full path to the video file.
     */
    std::string buildRoundVideoPath(const std::string& baseFolderPath, const std::string& gameName, int round) const;

    /**
     * @brief Checks if the initial trump card (Briscola) is still physically present on the table.
     * 
     * This method extracts only the first frame of the video to quickly determine 
     * if the static trump card has been picked up or is still lying on the table.
     * 
     * @param videoPath The path to the round's video file.
     * @return true if the trump card is still on the table (or if the video cannot be read).
     * @return false if the spot is empty or contains a different card.
     */
    bool isBriscolaStillOnTable(const std::string& videoPath);

    /**
     * @brief Identifies the trump card (Briscola) by analyzing the first three rounds.
     * 
     * @param gameName The name of the current game.
     * @param baseFolderPath The base directory containing the game videos.
     */
    void identifyBriscola(const std::string& gameName, const std::string& baseFolderPath);

    /**
     * @brief Processes a single round of the game.
     * 
     * Evaluates the video to recognize the cards played by North and South, 
     * determines the winner via the game engine, and logs the outcome.
     * 
     * @param roundNumber The current round number (1 to 20).
     * @param videoPath The path to the video file for this round.
     * @param gameName The name of the current game.
     */
    void playSingleRound(int roundNumber, const std::string& videoPath, const std::string& gameName);


    /**
     * @brief Logs the results of a completed round to the reporter.
     * 
     * Constructs a structured data package containing the identified cards, 
     * the current Briscola, and the round's outcome evaluated by the game engine. 
     * It then sends this data to the reporter and prints a brief summary to the console.
     * 
     * @param roundNumber The current round number.
     * @param playedCards An array containing the two cards played (index 0 for North, index 1 for South).
     * @param result The outcome of the round computed by the game engine (winner, leader, and points).
     */
    void recordRoundResults(int roundNumber, const Card playedCards[2], const RoundResult& result);
   
};

#endif // GAMEMANAGER_H