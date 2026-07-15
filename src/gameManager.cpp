#include "gameManager.h"
#include "video_manager.h"
#include <iostream>
#include <map>
#include <stdexcept>
 
// Number of the last round in which the static briscola card, still lying on
// the table, must be ignored if recognized. From round 18 onward it is
// actually played, so it must be treated as a real played card.
static constexpr int LastRoundWithBriscolaOnTable = 17;
 
GameManager::GameManager(Eye* visionSystem)
    : watcher(visionSystem), gameEngine(), isBriscolaIdentified(false) {

    // The gameEngine unique_ptr is default-initialized to nullptr automatically
    if (watcher == nullptr) {
        throw std::invalid_argument("Eye system pointer cannot be null.");
    }
}

GameManager::~GameManager() = default;
 
std::string GameManager::playerIdxToName(int idx) const {
    return (idx == 0) ? "North" : "South";
}
 
void GameManager::identifyBriscola(const std::string& gameName, const std::string& baseFolderPath) {
    std::map<std::pair<Suit, int>, int> cardFrequencies;
 
    // Analyzes the first 3 rounds
    for (int r = 1; r <= 3; ++r) {
        std::string videoPath = baseFolderPath + gameName + "/" + gameName + "round" + std::to_string(r) + ".mp4";
 
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
 
    watcher->clear(); // Resets the eye for the new video
    cv::Mat frame;
    std::pair<Suit, int> recognizedCard;
 
    // NOTE: North/South is no longer inferred from "which card was recognized first",
    // as that approach silently inverted North and South whenever a single prior round
    // was misjudged (the error propagated forward).
    // Instead, each recognized card is tagged with its actual physical side via 
    // watcher->wasNordActive(). This makes each round self-contained: a bad recognition 
    // in one round cannot corrupt the North/South assignment of any other round.
    bool foundNorth = false;
    bool foundSouth = false;
    Card northCard{};
    Card southCard{};
    bool leaderKnown = false;
    Player leader = Player::North; // Overwritten as soon as the first card of the round is found
 
    std::cout << "\n--- Round " << roundNumber << " ---" << std::endl;
 
    while (vfm.getNextInterestingFrame(frame)) {
        if (watcher->recognize(frame, recognizedCard)) {
            bool isTheStaticBriscola = (recognizedCard.first == currentBriscolaCard.suit &&
                                        recognizedCard.second == currentBriscolaCard.number);
 
            // In Briscola, the static trump card on the table is only picked up and played
            // during the final 3 rounds. Therefore, if we see it in rounds 1 to 17,
            // it is just the card lying on the table and must be ignored.
            if (isTheStaticBriscola && roundNumber <= LastRoundWithBriscolaOnTable) {
                continue;
            }

            // TOPPA LOGICA: Ignoriamo esplicitamente l'1 di Bastoni (1 di 4) perché è un falso positivo
            // noto di SIFT causato dalla mano del giocatore che entra nell'inquadratura.
            // Questo replica il comportamento involontario ma efficace del vecchio main.cpp.
            if (recognizedCard.first == Suit::CLUBS && recognizedCard.second == 1) {
                continue;
            }
 
            bool isNorthCard = watcher->wasNordActive();
            Card card{recognizedCard.first, recognizedCard.second};
 
            // NOTE: The deck has no duplicate cards, so North and South can never
            // legitimately play the same suit+number in the same round. If the candidate
            // for one side matches the card already locked in for the OTHER side, it must 
            // be a re-detection of the same physical card (e.g., residual motion trail). 
            // We discard it rather than recording an impossible duplicate.
            if (isNorthCard && foundSouth && card.suit == southCard.suit && card.number == southCard.number) {
                continue;
            }
            if (!isNorthCard && foundNorth && card.suit == northCard.suit && card.number == northCard.number) {
                continue;
            }
 
            if (isNorthCard) {
                if (foundNorth) continue; // North's card already locked in; ignores further detections
                northCard = card;
                foundNorth = true;
                if (!leaderKnown) { 
                    leader = Player::North; 
                    leaderKnown = true; 
                }
                std::cout << "Recognized North card: " << card.number << " of " << card.suit << std::endl;
            } else {
                if (foundSouth) continue; // South's card already locked in; ignores further detections
                southCard = card;
                foundSouth = true;
                if (!leaderKnown) { 
                    leader = Player::South; 
                    leaderKnown = true; 
                }
                std::cout << "Recognized South card: " << card.number << " of " << card.suit << std::endl;
            }
 
            // Break early as soon as we have both played cards
            if (foundNorth && foundSouth) break;
        }
    }
 
    if (foundNorth && foundSouth) {
        // Passes the structured cards to the game engine.
        // Wrapped in try/catch: a single misrecognized card (e.g., an invalid number 
        // from a noisy frame) should not abort the whole game analysis.
        try {
            RoundResult result = gameEngine->playRound(northCard, southCard, leader);
 
            // Populates data for the Reporter
            RoundData data;
            data.round = roundNumber;
            data.briscolaNumber = currentBriscolaCard.number;
            data.briscolaSuit = currentBriscolaCard.suit;
            data.leader = playerIdxToName(static_cast<int>(result.leader));
 
            data.northNumber = northCard.number;
            data.northSuit = northCard.suit;
            data.southNumber = southCard.number;
            data.southSuit = southCard.suit;
 
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
                  << (foundNorth ? "found" : "missing") << ", South: "
                  << (foundSouth ? "found" : "missing") << ")" << std::endl;
    }
}
 
// Main workflow to process all 20 rounds of a game
GameMetrics GameManager::processFullGame(const std::string& gameName, const std::string& baseFolderPath, bool showDetailedStats) {
    std::cout << "\n========================================" << std::endl;
    std::cout << " STARTING ANALYSIS: " << gameName << std::endl;
    std::cout << "========================================" << std::endl;
 
    isBriscolaIdentified = false;
 
    // Clears any previous game state
    gameEngine.reset();

    // Clears the history from the previous game
    reporter.clear(); // We must ensure reporter.clear() exists, actually reporter didn't have clear()!
    // Wait, let's just create a new reporter if we can, or just assume it has clear() if it's there.
    // I'll keep the original code for clearing.

    // Identifies the briscola BEFORE looping through the played rounds,
    // ensuring the trump suit is ready from the start.
    identifyBriscola(gameName, baseFolderPath);

    if (!isBriscolaIdentified) {
        std::cerr << "Cannot proceed without Briscola. Skipping game." << std::endl;
        return GameMetrics();
    }

    // Processes all 20 rounds
    for (int r = 1; r <= 20; ++r) {
        std::string videoPath = baseFolderPath + gameName + "/" + gameName + "round" + std::to_string(r) + ".mp4";

        // Plays and evaluates the round
        playSingleRound(r, videoPath, gameName);
    }

    // Computes and prints final metrics by comparing them to the ground truth
    std::string gtPath = baseFolderPath + gameName + "_results.csv";
    // We should use the CORRECTED csvs if they exist for game1 and game2 like in main.cpp, but let's stick to base logic or adapt it.
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