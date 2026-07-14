/*
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

int main() {
    std::string datasetPath = "../dataset/Briscola_Trentine";
    std::vector<std::tuple<cv::Mat, Suit, int>> dataset = loadDataset(datasetPath);
    if (dataset.empty()) {
        std::cerr << "Errore: Dataset non trovato in " << datasetPath << std::endl;
        return -1;
    }
    std::cout << "Dataset caricato: " << dataset.size() << " carte." << std::endl;

    Eye watcher;
    watcher.fit(dataset);

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

            // Determiniamo chi dovrebbe iniziare secondo la logica di gioco
            int leaderIdx = (game == nullptr) ? 0 : game->getNextFirstPlayer();
            std::string leaderName = playerIdxToName(leaderIdx);
            
            std::cout << "\n--- Round " << r << " (Leader: " << leaderName << ") ---" << std::endl;

            while (vfm.getNextInterestingFrame(frame)) {
                cv::imshow("frame pre rec", frame);

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
                    if (recognizedCard.first
                     == briscolaSuit && recognizedCard.second == briscolaNumber) {
                        // Se è la prima carta del round, è quasi certamente la briscola statica
                        if (playedInRound.empty()) continue;
                    }

                    // Se arriviamo qui, è una carta giocata dai player. 
                    // NON la aggiungiamo ancora al gioco, la salviamo e basta.
                    playedInRound.push_back(recognizedCard);
                    std::cout << "Riconosciuta carta " << playedInRound.size() << ": " << recognizedCard.second << " di " << recognizedCard.first << std::endl;
                    
                    if (playedInRound.size() == 2) break;
                }
            }

            // Una volta usati i frame, elaboriamo il risultato del round
            if (playedInRound.size() == 2) {
                Card northCard, southCard;

                // Mappiamo le carte trovate sui giocatori fisici (North/South)
                // in base a chi ha il turno di apertura
                if (leaderIdx == 0) { // North led
                    northCard = {playedInRound[0].first, playedInRound[0].second};
                    southCard = {playedInRound[1].first, playedInRound[1].second};
                } else { // South led
                    southCard = {playedInRound[0].first, playedInRound[0].second};
                    northCard = {playedInRound[1].first, playedInRound[1].second};
                }

                // Passiamo le carte strutturate al nuovo engine di gioco
                RoundResult result = game->playRound(northCard, southCard);

                // Popoliamo i dati per il Reporter
                RoundData data;
                data.round = r;
                data.briscolaNumber = briscolaNumber;
                data.briscolaSuit = briscolaSuit;
                data.leader = playerIdxToName(static_cast<int>(result.leader));
                
                data.northNumber = northCard.number; 
                data.northSuit = northCard.suit;
                data.southNumber = southCard.number; 
                data.southSuit = southCard.suit;

                data.winner = playerIdxToName(static_cast<int>(result.winner));
                data.points = result.points;

                reporter.logRound(data);
                std::cout << "Round " << r << " FINITO. Vincitore: " << data.winner << " (" << data.points << " pts)" << std::endl;
            } else {
                std::cout << "ERRORE: Round " << r << " incompleto (" << playedInRound.size() << "/2)" << std::endl;
            }
        }

        std::string gtPath = "../dataset/" + gameName + "results.csv";
        std::cout << "\n>>> FINE ANALISI " << gameName << ". Risultati finali:" << std::endl;
        reporter.calculateMetrics(gtPath);
        
        if (game) { delete game; game = nullptr; }
    }

    return 0;
}
*/