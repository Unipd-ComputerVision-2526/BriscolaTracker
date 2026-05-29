#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;
using namespace cv;
using namespace std;

// Struttura per memorizzare i dati di ogni carta template
struct CardTemplate {
    string name;
    Mat descriptors;
    vector<KeyPoint> keypoints;
};

int main() {
    // 1. Inizializza il detector SIFT
    Ptr<SIFT> sift = SIFT::create();
    
    // 2. Carica e calcola keypoints/descriptors per le 40 carte del dataset
    vector<CardTemplate> deck;
    string datasetPath = "dataset/Briscola_Trentine";
    
    cout << "Caricamento dataset carte da: " << datasetPath << "..." << endl;
    for (const auto& entry : fs::directory_iterator(datasetPath)) {
        if (entry.path().extension() == ".JPG" || entry.path().extension() == ".jpg") {
            Mat img = imread(entry.path().string(), IMREAD_GRAYSCALE);
            if (img.empty()) continue;

            CardTemplate card;
            card.name = entry.path().filename().string();
            sift->detectAndCompute(img, noArray(), card.keypoints, card.descriptors);
            deck.push_back(card);
        }
    }
    cout << "Caricate " << deck.size() << " carte nel dataset." << endl;

    // Inizializza il matcher (FLANN è ottimo per i descrittori SIFT che sono float)
    FlannBasedMatcher matcher;

    // 3. Analizza i 4 game
    for (int i = 1; i <= 4; ++i) {
        string videoPath = "dataset/game" + to_string(i) + "/game" + to_string(i) + "round1.mp4";
        VideoCapture cap(videoPath);
        
        if (!cap.isOpened()) {
            cerr << "Errore: Impossibile aprire il video " << videoPath << endl;
            continue;
        }

        Mat frame;
        cap >> frame; // Legge il primo frame
        if (frame.empty()) {
            cerr << "Errore: Frame vuoto in " << videoPath << endl;
            continue;
        }

        cout << "\nAnalisi Game " << i << " (" << videoPath << ")..." << endl;

        // Converti il frame in scala di grigi per l'estrazione delle feature
        Mat grayFrame;
        cvtColor(frame, grayFrame, COLOR_BGR2GRAY);

        vector<KeyPoint> frameKeypoints;
        Mat frameDescriptors;
        sift->detectAndCompute(grayFrame, noArray(), frameKeypoints, frameDescriptors);

        string bestCardName = "Nessuna";
        int maxGoodMatches = 0;

        // 4. Confronta il frame con ogni carta del mazzo
        for (const auto& card : deck) {
            vector<vector<DMatch>> knnMatches;
            if (card.descriptors.empty() || frameDescriptors.empty()) continue;
            
            // Trova le 2 migliori corrispondenze per ogni feature
            matcher.knnMatch(card.descriptors, frameDescriptors, knnMatches, 2);

            // 5. Applica il Lowe's Ratio Test per filtrare i falsi positivi
            const float ratio_thresh = 0.75f;
            int goodMatchesCount = 0;
            for (size_t j = 0; j < knnMatches.size(); j++) {
                if (knnMatches[j][0].distance < ratio_thresh * knnMatches[j][1].distance) {
                    goodMatchesCount++;
                }
            }

            // Aggiorna la carta con il maggior numero di corrispondenze
            if (goodMatchesCount > maxGoodMatches) {
                maxGoodMatches = goodMatchesCount;
                bestCardName = card.name;
            }
        }

        cout << "-> Briscola rilevata: " << bestCardName 
             << " (Match validi: " << maxGoodMatches << ")" << endl;
    }

    return 0;
}