/**
 * @file gameManager.cpp
 * @brief Implementation of the class GameManager for the managing
 *        Briscola rounds.
 * @author Caterina Dri
 */

#include "gameManager.h"
#include "videoFrameManager.h"
#include <iostream>
#include <map>
#include <stdexcept>
#include <cstdlib>
#include <ctime>

// Number of the last round in which the static briscola card, still lying on
// the table, must be ignored if recognized. From round 18 onward it is
// actually played, so it must be treated as a real played card.
static constexpr int LastRoundWithBriscolaOnTable = 17;

// ============================================================================
// CONSTRUCTOR & DESTRUCTOR
// ============================================================================

GameManager::GameManager(Eye* visionSystem)
    : watcher(visionSystem), gameEngine(), isBriscolaIdentified(false) {

    // The gameEngine unique_ptr is default-initialized to nullptr automatically
    if (watcher == nullptr) {
        throw std::invalid_argument("Eye system pointer cannot be null.");
    }
}

GameManager::~GameManager() = default;

// ============================================================================
// PUBLIC METHODS
// ============================================================================

GameMetrics GameManager::processFullGame(const std::string& gameName, const std::string& datasetFolderPath, const std::string& resultsFolderPath, bool showDetailedStats){
    std::cout << "\n========================================" << std::endl;
    std::cout << " STARTING ANALYSIS: " << gameName << std::endl;
    std::cout << "========================================" << std::endl;
 
    // Reset internal state, engine, and reporting history from any previously analyzed games
    isBriscolaIdentified = false;
    briscolaPickedUp = false;
    gameEngine.reset();
    reporter.clear();

    // Identify the Briscola card before processing rounds to configure the game engine's trump suit
    identifyBriscola(gameName, datasetFolderPath);

    // Process all 20 rounds of a standard Briscola match
    for (int r = 1; r <= 20; ++r) {
        std::string videoPath = buildRoundVideoPath(datasetFolderPath, gameName, r);

        // Plays and evaluates the round
        playSingleRound(r, videoPath, gameName);
    }

    // Export data round to csv
    
    std::string outCsvPath = resultsFolderPath + gameName + "Output.csv";
    reporter.exportCSV(outCsvPath);
    std::cout << ">>> Exported tracker results to: " << outCsvPath << std::endl;

    // Computes and prints final metrics by comparing them to the ground truth
    std::string gtPath = datasetFolderPath + gameName + "Results.csv";
    // Since the ground truth data provided have different names and have been 
    // corrected, we added multiple options to dynamically find the right file.
    std::vector<std::string> possibleCorrectedNames = {
        datasetFolderPath + gameName + "_results_corrected.csv", // Ideal format
        datasetFolderPath + gameName + "resultsCORRECTED.csv",   // Legacy format (no underscore)
        datasetFolderPath + gameName + "resultsCORRECTED 2.csv"  // Legacy format (with space)
    };

    for (const std::string& checkPath : possibleCorrectedNames) {
        if (std::filesystem::exists(checkPath)) {
            gtPath = checkPath;
            break; 
        }
    }
    
    std::cout << "\n>>> END OF ANALYSIS for " << gameName << ". Final Results:" << std::endl;
    return reporter.calculateMetrics(gtPath, showDetailedStats);
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

std::string GameManager::playerIdxToName(int idx) const {
    std::string playerName;
    playerName = (idx == 0) ? "North" : "South";
    return playerName;
}

std::string GameManager::buildRoundVideoPath(const std::string& baseFolderPath, const std::string& gameName, int round) const {
    std::string videoPath = baseFolderPath + gameName + "/" + gameName + "round" + std::to_string(round) + ".mp4";
    return videoPath;
}

bool GameManager::isBriscolaStillOnTable(const std::string& videoPath) {
    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        return true; // can't verify: conservative default, avoid a false play
    }

    cv::Mat frame;
    int framesToCheck = 10; 
    int framesAnalyzed = 0;

    // Check only the first few frames to optimize performance. 
    // If the static Briscola is on the table, it should be immediately visible.
    while (framesAnalyzed < framesToCheck) {
        cap >> frame;
        if (frame.empty()) {
            break; 
        }

        std::pair<Suit, int> recognized;
        // If the detected card matches the known Briscola, it hasn't been drawn yet
        if (watcher->recognize(frame, recognized)) {
            if (recognized.first == currentBriscolaCard.suit && 
                recognized.second == currentBriscolaCard.number) {
                return true; 
            }
        }
        framesAnalyzed++;
    }

    // If the Briscola is not found in the initial frames, a player must have picked it up
    return false;
}


void GameManager::identifyBriscola(const std::string& gameName, const std::string& baseFolderPath) {
    std::map<std::pair<Suit, int>, int> cardFrequencies;

    int roundsToAnalyze = 5;
 
    // Analyze the first frame of the initial rounds to identify the static Briscola on the table
    for (int r = 1; r <= roundsToAnalyze; ++r) {
        std::string videoPath = buildRoundVideoPath(baseFolderPath, gameName, r);
 
        VideoFrameManager vfm(videoPath, 1);
 
        if (!vfm.isOpened()) {
            std::cerr << ">>> Warning: Cannot open video for Briscola identification: " << videoPath << std::endl;
            continue; 
        }
 
        cv::Mat firstFrame;
 
        if (vfm.getNextInterestingFrame(firstFrame)) {
            
            watcher->clear();
     
            std::pair<Suit, int> recognizedCard;

            if (watcher->recognize(firstFrame, recognizedCard)) {
                cardFrequencies[recognizedCard]++;
            }
        } else {
            std::cerr << ">>> Warning: Could not extract frame for video: " << videoPath << std::endl;
        }
    }
 
    int suitCounts[5] = {0, 0, 0, 0, 0};

    // Select the most frequently detected card to mitigate single-frame recognition errors
    int maxFreq = -1;
    std::pair<Suit, int> bestCard;
    
    for (const std::pair<const std::pair<Suit, int>, int>& entry : cardFrequencies) {
        if (entry.second > maxFreq) {
            maxFreq = entry.second;
            bestCard = entry.first; 
        }
        // Accumulate counts for the suit (entry.first.first è il seme)
        suitCounts[entry.first.first] += entry.second;
    }

    // Determine the most frequent suit from accumulated counts
    int maxSuitFreq = -1;
    Suit bestSuit = COINS;
    for (int s = 1; s <= 4; s++) {
        if (suitCounts[s] > maxSuitFreq) {
            maxSuitFreq = suitCounts[s];
            bestSuit = static_cast<Suit>(s);
        }
    }
 
    std::srand(std::time(nullptr));

    // Fallback hierarchy: 
    // 1. Full card identified -> 2. Only suit identified -> 3. Total random
    if (maxFreq > 0) {
        currentBriscolaCard.suit = bestCard.first;  
        currentBriscolaCard.number = bestCard.second;
        isBriscolaIdentified = true;
        std::cout << ">>> Briscola fully identified: " << bestCard.second << " of " << bestCard.first << std::endl;
    } 
    else if (maxSuitFreq > 0) {
        currentBriscolaCard.suit = bestSuit;
        currentBriscolaCard.number = (std::rand() % 10) + 1; // Random number, correct suit
        isBriscolaIdentified = false;
        std::cout << ">>> Briscola SUIT identified: " << bestSuit << " (Number guessed randomly)" << std::endl;
    } 
    else {
        currentBriscolaCard.suit = static_cast<Suit>((std::rand() % 4) + 1);
        currentBriscolaCard.number = (std::rand() % 10) + 1;
        isBriscolaIdentified = false;
        std::cout << ">>> ERROR: Could not identify Briscola. Generating totally random card." << std::endl;
    }

    gameEngine = std::make_unique<Briscola>(currentBriscolaCard.suit, 2);
}

void GameManager::playSingleRound(int roundNumber, const std::string& videoPath, const std::string& gameName) {

    // Initialize VideoFrameManager with motion-based selection to skip static frames
    VideoFrameManager vfm(videoPath);
    if (!vfm.isOpened()) {
        std::cerr << ">>> ERROR: Cannot open video for round"
                << roundNumber << " to path: " << videoPath << std::endl;
        return;
    }

    // Clear vision system state to prevent contamination from previous rounds
    watcher->clear(); 

    // Track if the static Briscola on the table has been drawn
    if (roundNumber <= LastRoundWithBriscolaOnTable) {
        briscolaPickedUp = false;
    }else{
        if (!briscolaPickedUp) {
            briscolaPickedUp = !isBriscolaStillOnTable(videoPath);
        }
    }

    cv::Mat frame;
    std::pair<Suit, int> recognizedCard;

    // State tracking: index 0 = North, index 1 = South
    bool found[2] = {false, false};
    Card playedCards[2];
    bool leaderKnown = false;
    Player leader = Player::North;

    std::cout << "\n--- Round " << roundNumber << " ---" << std::endl;

    while (vfm.getNextInterestingFrame(frame)) {
        if (watcher->recognize(frame, recognizedCard)) {
            bool isTheStaticBriscola = (recognizedCard.first == currentBriscolaCard.suit &&
                                        recognizedCard.second == currentBriscolaCard.number);

            // Ignore the static Briscola on the table until it is actually drawn
            if (isTheStaticBriscola && !briscolaPickedUp) {
                continue;
            }

            // Determine player indices and perform the check over X consecutive frames to filter out tracking noise
            int northVotes = 0;
            int southVotes = 0;

            for(int i = 0; i < 5; ++i) {
                if(watcher->wasNordActive()) northVotes++;
                else southVotes++;
            }

            // Decide the active player based on the majority of votes
            int myIdx = (watcher->wasNordActive()) ? 0 : 1;
            
            int otherIdx = 1 - myIdx; 
            Card card{recognizedCard.first, recognizedCard.second};

            // Duplicate Prevention: Discard the card if it matches the other player's 
            // locked card to avoid motion trail or tracking artifacts.
            if (found[otherIdx] && card.suit == playedCards[otherIdx].suit && card.number == playedCards[otherIdx].number) {
                continue;
            }

            // Player Swap Correction: If the current player's slot is full but a new card 
            // is detected, attribute it to the other player (handles crossed hands).
            if (found[myIdx]) {
                if (card.suit != playedCards[myIdx].suit || card.number != playedCards[myIdx].number) {
                    
                    if (!found[otherIdx]) {
                        playedCards[otherIdx] = card;
                        found[otherIdx] = true;
                    }
                }
                continue; 
            }

            // Lock-in & Leader Assignment
            playedCards[myIdx] = card;
            found[myIdx] = true;
            
            // The first player to drop a valid card becomes the leader
            if (!leaderKnown) { 
                Player visualLeader = static_cast<Player>(myIdx);

                if (visualLeader != previousWinner) {
                    std::cout << ">>> WARNING: Leader mismatch! Video: " 
                                << playerIdxToName(static_cast<int>(visualLeader)) 
                                << " | Rules: " 
                                << playerIdxToName(static_cast<int>(previousWinner)) << std::endl;

                }
                leader = visualLeader;
                leaderKnown = true; 
            }
            
            std::cout << "Recognized " << playerIdxToName(myIdx) << " card: " 
                      << card.number << " of " << card.suit << std::endl;

            // Stop processing once both cards are found
            if (found[0] && found[1]) break;
        }
    }

    if (found[0] && found[1]) {
        //Both cards identified by the vision system.
        try {
            RoundResult result = gameEngine->playRound(playedCards[0], playedCards[1], leader);
            
            previousWinner = result.winner; 
            isFirstRound = false;

            // Calls the helper function to pack and send data to reporter
            recordRoundResults(roundNumber, playedCards, result);

        } catch (const std::exception& e) {
            std::cerr << ">>> ERROR: Round " << roundNumber
                      << " could not be resolved (" << e.what() << "). Skipping." << std::endl;
        }
    } else {
        std::cout << ">>> ERROR: Round " << roundNumber << " incomplete. Applying fallback." << std::endl;
        
        // Assign a non-trump suit to the placeholder to avoid unintended point manipulation
        Suit dummySuit = (currentBriscolaCard.suit == Suit::COINS) ? Suit::CUPS : Suit::COINS;
        
        // Fill missing card slots with a low-value placeholder (2 = 0 points)
        // to satisfy engine requirements without influencing valid point calculations
        for (int i = 0; i < 2; ++i) {
            if (!found[i]) {
                playedCards[i] = Card{dummySuit, 2}; 
            }
        }
        
        try {
            // Determine leader based on game history if visual leader detection failed
            if (!leaderKnown) {
                leader = (isFirstRound) ? Player::North : previousWinner;
            }
            
            RoundResult result = gameEngine->playRound(playedCards[0], playedCards[1], leader);
            
            previousWinner = result.winner;
            isFirstRound = false;
            
            //Reset unidentified card values to 0 before logging to maintain data integrity in the final report
            for (int i = 0; i < 2; ++i) {
                if (!found[i]) playedCards[i].number = 0; 
            }
            
            recordRoundResults(roundNumber, playedCards, result);
            
        } catch (const std::exception& e) {
             std::cerr << ">>> ERROR: Fallback failed: " << e.what() << std::endl;
        }
    }
}

void GameManager::recordRoundResults(int roundNumber, const Card playedCards[2], const RoundResult& result) {
    RoundData data;
    data.round = roundNumber;
    data.briscolaNumber = currentBriscolaCard.number;
    data.briscolaSuit = currentBriscolaCard.suit;
    
    // Populates player decisions and game outcomes
    data.leader = playerIdxToName(static_cast<int>(result.leader));
    data.northNumber = playedCards[0].number;
    data.northSuit = playedCards[0].suit;
    data.southNumber = playedCards[1].number;
    data.southSuit = playedCards[1].suit;

    data.winner = playerIdxToName(static_cast<int>(result.winner));
    data.points = result.points;

    // Sends the structured data to the reporter
    reporter.logRound(data);
    
    std::cout << "Round " << roundNumber << " FINISHED. Leader: " << data.leader
              << ". Winner: " << data.winner << " (" << data.points << " pts)" << std::endl;
}
 


