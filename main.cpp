#include <iostream>
#include <vector>
#include <tuple>
#include <string>
#include <opencv2/opencv.hpp>
#include <utils.h>
#include <video_manager.h>
#include <eye.h>
#include <briscola.h>
#include <reporter.h>

std::string playerIdxToName(int idx) {
    return (idx == 0) ? "North" : "South";
}

int main(int argc, char* argv[]) {
    bool showDetailedStats = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-stat") {
            showDetailedStats = true;
        }
    }
    std::string datasetPath = "../dataset/Briscola_Trentine";
    std::vector<std::tuple<cv::Mat, Suit, int>> dataset = loadDataset(datasetPath);
    if (dataset.empty()) {
        std::cerr << "Errore: Dataset non trovato in " << datasetPath << std::endl;
        return -1;
    }
    std::cout << "Dataset caricato: " << dataset.size() << " carte." << std::endl;

    Eye watcher;
    watcher.fit(dataset);

    GameMetrics totalMetrics;

    for (int g = 1; g <= 4; ++g) {
        std::string gameName = "game" + std::to_string(g);
        std::cout << "\n========================================" << std::endl;
        std::cout << " ANALISI " << gameName << std::endl;
        std::cout << "========================================" << std::endl;

        Reporter reporter;
        Briscola* game = nullptr;
        Suit briscolaSuit = COINS;
        int briscolaNumber = 0;
        bool briscolaInitialized = false;

        for (int r = 1; r <= 20; ++r) {
            std::string videoPath = "../dataset/" + gameName + "/" + gameName + "round" + std::to_string(r) + ".mp4";
            VideoFrameManager vfm(videoPath, 10); // Alzata leggermente la soglia movimento

            if (!vfm.isOpened()) continue;

            watcher.clear(); // Reset per ogni nuovo video
            cv::Mat frame;
            std::pair<Suit, int> recognizedCard;
            std::vector<std::pair<Suit, int>> playedInRound;
            std::vector<bool> playerOfCard;

            // Determiniamo chi dovrebbe iniziare secondo la logica di gioco
            int leaderIdx = (game == nullptr) ? 0 : game->getNextFirstPlayer();
            std::string leaderName = playerIdxToName(leaderIdx);
            
            std::cout << "\n--- Round " << r << " (Leader: " << leaderName << ") ---" << std::endl;

            while (vfm.getNextInterestingFrame(frame)) {
                // cv::imshow("frame pre rec", frame);
                // cv::waitKey(0);

                if (watcher.recognize(frame, recognizedCard)) {
                    // La prima carta che vediamo in ogni round (solitamente) è la Briscola sul tavolo.
                    // Se non abbiamo ancora inizializzato la briscola ufficiale del game:
                    if (!briscolaInitialized) {
                        briscolaSuit = recognizedCard.first;
                        briscolaNumber = recognizedCard.second;
                        briscolaInitialized = true;
                        game = new Briscola(briscolaSuit, 2);
                        std::cout << "Briscola identificata: " << briscolaNumber << " di " << briscolaSuit << std::endl;
                        continue; 
                    }

                    // Se abbiamo già la briscola e questa carta è proprio la briscola, 
                    // la ignoriamo (è solo il watcher che l'ha usata per calibrare lastMask_)
                    if (recognizedCard.first == briscolaSuit && recognizedCard.second == briscolaNumber) {
                        // Se è la prima carta del round, è quasi certamente la briscola statica
                        if (playedInRound.empty()) continue;
                    }

                    // Se arriviamo qui, è una carta giocata dai player
                    bool isNord = watcher.wasNordActive();
                    if (!playerOfCard.empty() && playerOfCard.back() == isNord) continue; // Salta i duplicati dello stesso player

                    playedInRound.push_back(recognizedCard);
                    playerOfCard.push_back(isNord);
                    game->addCardToRound(recognizedCard.first, recognizedCard.second);
                    std::cout << "Riconosciuta carta " << playedInRound.size() << ": " << recognizedCard.second << " di " << recognizedCard.first << std::endl;
                    
                    if (playedInRound.size() == 2) break;
                }
            }

            if (playedInRound.size() == 2) {
                RoundData data;
                data.round = r;
                data.briscolaNumber = briscolaNumber;
                data.briscolaSuit = briscolaSuit;
                data.leader = leaderName;
                
                for (size_t i = 0; i < 2; ++i) {
                    if (playerOfCard[i]) {
                        data.northNumber = playedInRound[i].second; data.northSuit = playedInRound[i].first;
                    } else {
                        data.southNumber = playedInRound[i].second; data.southSuit = playedInRound[i].first;
                    }
                }

                // Chi vince inizia il prossimo round (estratto dall'oggetto game aggiornato)
                int winnerIdx = game->getNextFirstPlayer();
                data.winner = playerIdxToName(winnerIdx);
                data.points = game->getLastRoundPoints();

                reporter.logRound(data);
                std::cout << "Round " << r << " FINITO. Vincitore: " << data.winner << " (" << data.points << " pts)" << std::endl;
            } else {
                std::cout << "ERRORE: Round " << r << " incompleto (" << playedInRound.size() << "/2)" << std::endl;
            }
        }

        std::string gtPath = "../dataset/" + gameName + "resultsCORRECTED.csv";
        if (g == 2) {
            gtPath = "../dataset/" + gameName + "resultsCORRECTED 2.csv";
        }
        std::cout << "\n>>> FINE ANALISI " << gameName << ". Risultati finali:" << std::endl;
        GameMetrics gm = reporter.calculateMetrics(gtPath, showDetailedStats);
        totalMetrics.add(gm);
        
        if (game) { delete game; game = nullptr; }
    }

    auto formatPct = [](int count, int total) {
        if (total == 0) return std::string("-");
        double pct = (static_cast<double>(count) / total) * 100.0;
        char buf[32];
        snprintf(buf, sizeof(buf), "%d (%.1f%%)", count, pct);
        return std::string(buf);
    };

    std::cout << "\n========================================" << std::endl;
    std::cout << " RISULTATI GLOBALI (TUTTI I GAME)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Card Recognition Accuracy: " << (totalMetrics.expectedCards > 0 ? (static_cast<double>(totalMetrics.correctCards) / totalMetrics.expectedCards) * 100.0 : 0.0) << "%" << std::endl;
    std::cout << "Player Identification Accuracy: " << (totalMetrics.totalPlayers > 0 ? (static_cast<double>(totalMetrics.correctPlayers) / totalMetrics.totalPlayers) * 100.0 : 0.0) << "%" << std::endl;
    std::cout << "Briscola Recognition Accuracy: " << (totalMetrics.expectedBriscola > 0 ? (static_cast<double>(totalMetrics.correctBriscola) / totalMetrics.expectedBriscola) * 100.0 : 0.0) << "%" << std::endl;
    std::cout << "Rounds Evaluated: " << totalMetrics.totalEvaluated << " / " << totalMetrics.expectedBriscola << std::endl;

    if (showDetailedStats) {
        std::cout << "\n--- DETAILED SUIT METRICS (GLOBAL) ---\n";
        std::cout << std::left << std::setw(10) << "SUIT" 
                  << std::setw(15) << "totale atteso"
                  << std::setw(20) << "Seme corretto"
                  << std::setw(25) << "Seme + Numero esatti"
                  << std::setw(20) << "Seme errato"
                  << std::setw(20) << "Round incompleto"
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

    return 0;
}
