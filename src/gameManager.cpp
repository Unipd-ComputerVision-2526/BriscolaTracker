/**
 * @file gameManager.cpp
 * @brief Implementation of the class GameManager for the managing
 *        Briscola rounds.
 * @author Caterina Dri
 */

#include "gameManager.h"
#include "video_manager.h"
#include <iostream>
#include <map>
#include <stdexcept>
#include <random>

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

void GameManager::processFullGame(const std::string& gameName, const std::string& baseFolderPath) {
    std::cout << "\n========================================" << std::endl;
    std::cout << " STARTING ANALYSIS: " << gameName << std::endl;
    std::cout << "========================================" << std::endl;
 
    // Reset internal state, engine, and reporting history from any previously analyzed games
    isBriscolaIdentified = false;
    briscolaPickedUp = false;
    gameEngine.reset();
    reporter.clear();

    // Identify the Briscola card before processing rounds to configure the game engine's trump suit
    identifyBriscola(gameName, baseFolderPath);

    // Fallback: If the vision system fails to find the Briscola, generate a random valid card.
    // This ensures the game engine can still initialize and track standard card drops, 
    // preserving basic card and player recognition metrics despite the missing trump context.
    if (!isBriscolaIdentified) {
        std::cerr << ">>> WARNING: Cannot identify Briscola. Generating a random Card to continue the analysis." << std::endl;
        
        std::srand(std::time(nullptr));
        
        int randomSuit = (std::rand() % 4) + 1; 
        
        int randomNumber = (std::rand() % 10) + 1;
        
        currentBriscolaCard.suit = static_cast<Suit>(randomSuit);
        currentBriscolaCard.number = randomNumber;
        
        // Initialize the game engine with the randomly generated Briscola
        gameEngine = std::make_unique<Briscola>(currentBriscolaCard.suit, 2);
    }

    // Process all 20 rounds of a standard Briscola match
    for (int r = 1; r <= 20; ++r) {
        std::string videoPath = buildRoundVideoPath(baseFolderPath, gameName, r);

        // Plays and evaluates the round
        playSingleRound(r, videoPath, gameName);
    }

    // Compute and display final evaluation metrics against the ground truth dataset
    std::string gtPath = baseFolderPath + gameName + "_results.csv";
    std::cout << "\n>>> END OF ANALYSIS for " << gameName << ". Final Results:" << std::endl;
    reporter.calculateMetrics(gtPath);
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
 
    // Select the most frequently detected card to mitigate single-frame recognition errors
    int maxFreq = -1;
    std::pair<Suit, int> bestCard;
    for (const std::pair<const std::pair<Suit, int>, int>& pair: cardFrequencies) {
        if (pair.second > maxFreq) {
            maxFreq = pair.second;
            bestCard = pair.first;
        }
    }
 
    if (maxFreq > 0) {
        currentBriscolaCard.suit = bestCard.first;
        currentBriscolaCard.number = bestCard.second;
        isBriscolaIdentified = true;
 
        // Initializes the game engine with the identified Briscola suit
        gameEngine = std::make_unique<Briscola>(currentBriscolaCard.suit, 2);
 
        std::cout << ">>> Briscola correctly identified: "
                  << currentBriscolaCard.number << " of " << currentBriscolaCard.suit
                  << " (seen in " << maxFreq << " frames across first " << roundsToAnalyze << " rounds)" << std::endl;
    } else {
        std::cerr << ">>> ERROR: Could not identify any card as Briscola in the firsts rounds!" << std::endl;
    }
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

            // Determine player indices
            int myIdx = watcher->wasNordActive() ? 0 : 1;
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
                leader = (myIdx == 0) ? Player::North : Player::South; 
                leaderKnown = true; 
            }
            
            std::cout << "Recognized " << playerIdxToName(myIdx) << " card: " 
                      << card.number << " of " << card.suit << std::endl;

            // Stop processing once both cards are found
            if (found[0] && found[1]) break;
        }
    }

    if (found[0] && found[1]) {
        try {
            RoundResult result = gameEngine->playRound(playedCards[0], playedCards[1], leader);
            
            // Calls the helper function to pack and send data to reporter
            recordRoundResults(roundNumber, playedCards, result);

        } catch (const std::exception& e) {
            std::cerr << ">>> ERROR: Round " << roundNumber
                      << " could not be resolved (" << e.what() << "). Skipping." << std::endl;
        }
    } else {
        std::cout << "ERROR: Round " << roundNumber << " incomplete (North: "
                  << (found[0] ? "found" : "missing") << ", South: "
                  << (found[1] ? "found" : "missing") << ")" << std::endl;
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
 
