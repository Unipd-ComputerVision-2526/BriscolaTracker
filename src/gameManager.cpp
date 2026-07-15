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

// Main workflow to process all 20 rounds of a game
GameMetrics GameManager::processFullGame(const std::string& gameName, const std::string& baseFolderPath, bool showDetailedStats) {
    std::cout << "\n========================================" << std::endl;
    std::cout << " STARTING ANALYSIS: " << gameName << std::endl;
    std::cout << "========================================" << std::endl;
 
    isBriscolaIdentified = false;
    briscolaPickedUp = false;
 
    // Clears any previous game state
    gameEngine.reset();

    // Clears the history from the previous game
    reporter.clear();

    // Identifies the briscola BEFORE looping through the played rounds,
    // ensuring the trump suit is ready from the start.
    identifyBriscola(gameName, baseFolderPath);

    if (!isBriscolaIdentified) {
        std::cerr << "Cannot proceed without Briscola. Skipping game." << std::endl;
        return GameMetrics();
    }

    // Processes all 20 rounds
    for (int r = 1; r <= 20; ++r) {
        std::string videoPath = buildRoundVideoPath(baseFolderPath, gameName, r);

        // Plays and evaluates the round
        playSingleRound(r, videoPath, gameName);
    }

    // Computes and prints final metrics by comparing them to the ground truth
    std::string gtPath = baseFolderPath + gameName + "_results.csv";
    std::string gtPath2 = baseFolderPath + gameName + "resultsCORRECTED.csv";
    if (gameName == "game2") {
        gtPath2 = baseFolderPath + gameName + "resultsCORRECTED 2.csv";
    }
    if (std::filesystem::exists(gtPath2)) {
        gtPath = gtPath2;
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

    cv::Mat firstFrame;
    cap >> firstFrame;
    if (firstFrame.empty()) {
        return true;
    }

    std::pair<Suit, int> recognized;
    if (!watcher->recognize(firstFrame, recognized)) {
        return false; // nothing recognizable in that spot -> already picked up
    }

    bool isStillOnTable = (recognized.first == currentBriscolaCard.suit && 
                           recognized.second == currentBriscolaCard.number);

    return isStillOnTable;
}
 
void GameManager::identifyBriscola(const std::string& gameName, const std::string& baseFolderPath) {
    std::map<std::pair<Suit, int>, int> cardFrequencies;
 
    // Analyzes the first 3 rounds
    for (int r = 1; r <= 3; ++r) {
        std::string videoPath = buildRoundVideoPath(baseFolderPath, gameName, r);
 
        // NOTE: Using the 1-argument constructor here restores motion-based frame selection
        // (frameSkip=10, threshold=5.0 by default). This catches the exact moment a card 
        // appears/changes instead of sampling arbitrary, evenly spaced frames.
        VideoFrameManager vfm(videoPath);
 
        if (!vfm.isOpened()) {
            std::cerr << ">>> Warning: Cannot open video for Briscola identification: " << videoPath << std::endl;
            continue; // If a video is not found, skip and try the next one
        }
 
        cv::Mat frame;
        int framesAnalyzed = 0;
        watcher->clear();
 
        // Reads up to 20 frames for each of the 3 videos (60 frames total)
        while (framesAnalyzed < 20 && vfm.getNextInterestingFrame(frame)) {
            std::pair<Suit, int> recognizedCard;
            if (watcher->recognize(frame, recognizedCard)) {
                cardFrequencies[recognizedCard]++;
            }
            framesAnalyzed++;
        }
    }
 
    // Finds the card with the highest frequency across all 3 rounds
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
 
        // Initializes the game engine with the identified briscola
        gameEngine = std::make_unique<Briscola>(currentBriscolaCard.suit, 2);
 
        std::cout << ">>> Briscola correctly identified: "
                  << currentBriscolaCard.number << " of " << currentBriscolaCard.suit
                  << " (seen in " << maxFreq << " frames across first 3 rounds)" << std::endl;
    } else {
        std::cerr << ">>> ERROR: Could not identify any card as Briscola in the first 3 rounds!" << std::endl;
    }
}

void GameManager::playSingleRound(int roundNumber, const std::string& videoPath, const std::string& gameName) {
    // Same approach as in identifyBriscola: use the 1-argument constructor to 
    // enable motion-based frame selection.
    VideoFrameManager vfm(videoPath);
    if (!vfm.isOpened()) {
        std::cerr << ">>> ERROR: Cannot open video for round"
                << roundNumber << " to path: " << videoPath << std::endl;
        return;
    }

    watcher->clear(); // reset before the table check, so residualImage_ stays consistent for this round

    // Detects, round by round, whether the trump card has actually been picked
    // up and played. Once confirmed off the table, it never needs checking again this game.
    if (!briscolaPickedUp) {
        briscolaPickedUp = !isBriscolaStillOnTable(videoPath);
    }

    cv::Mat frame;
    std::pair<Suit, int> recognizedCard;

    // State tracking: index 0 = North, index 1 = South
    bool found[2] = {false, false};
    Card playedCards[2];
    bool leaderKnown = false;
    Player leader = Player::North; // Overwritten as soon as the first card of the round is found

    std::cout << "\n--- Round " << roundNumber << " ---" << std::endl;

    while (vfm.getNextInterestingFrame(frame)) {
        if (watcher->recognize(frame, recognizedCard)) {
            bool isTheStaticBriscola = (recognizedCard.first == currentBriscolaCard.suit &&
                                        recognizedCard.second == currentBriscolaCard.number);

            // The static trump card lying on the table must be ignored until it is
            // actually picked up and played (see isBriscolaStillOnTable).
            if (isTheStaticBriscola && !briscolaPickedUp) {
                continue;
            }

            // TOPPA LOGICA: Ignoriamo esplicitamente l'1 di Bastoni (1 di 4) perché è un falso positivo
            // noto di SIFT causato dalla mano del giocatore che entra nell'inquadratura.
            // Questo replica il comportamento involontario ma efficace del vecchio main.cpp.
            if (recognizedCard.first == Suit::CLUBS && recognizedCard.second == 1) {
                continue;
            }
 
            // Determine player index: 0 for North, 1 for South
            int myIdx = watcher->wasNordActive() ? 0 : 1;
            int otherIdx = 1 - myIdx; 
            Card card{recognizedCard.first, recognizedCard.second};

            // NOTE: The deck has no duplicate cards, so North and South can never
            // legitimately play the same suit+number in the same round. If the candidate
            // for one side matches the card already locked in for the OTHER side, it must 
            // be a re-detection of the same physical card (e.g., residual motion trail). 
            // We discard it rather than recording an impossible duplicate.
            if (found[otherIdx] && card.suit == playedCards[otherIdx].suit && card.number == playedCards[otherIdx].number) {
                continue;
            }

            if (found[myIdx]) {
                continue; // this side's card already locked in; ignores further detections
            }

            // Lock in the card
            playedCards[myIdx] = card;
            found[myIdx] = true;
            
            if (!leaderKnown) { 
                leader = (myIdx == 0) ? Player::North : Player::South; 
                leaderKnown = true; 
            }
            
            std::cout << "Recognized " << playerIdxToName(myIdx) << " card: " 
                      << card.number << " of " << card.suit << std::endl;

            // Break early as soon as we have both played cards
            if (found[0] && found[1]) break;
        }
    }

    if (found[0] && found[1]) {
        // Passes the structured cards to the game engine.
        // Wrapped in try/catch: a single misrecognized card (e.g., an invalid number 
        // from a noisy frame) should not abort the whole game analysis.
        try {
            RoundResult result = gameEngine->playRound(playedCards[0], playedCards[1], leader);

            // Populates data for the Reporter
            RoundData data;
            data.round = roundNumber;
            data.briscolaNumber = currentBriscolaCard.number;
            data.briscolaSuit = currentBriscolaCard.suit;
            data.leader = playerIdxToName(static_cast<int>(result.leader));

            data.northNumber = playedCards[0].number;
            data.northSuit = playedCards[0].suit;
            data.southNumber = playedCards[1].number;
            data.southSuit = playedCards[1].suit;

            data.winner = playerIdxToName(static_cast<int>(result.winner));
            data.points = result.points;

            reporter.logRound(data);
            std::cout << "Round " << roundNumber << " FINISHED. Leader: " << data.leader
                      << ". Winner: " << data.winner << " (" << data.points << " pts)" << std::endl;
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
 
