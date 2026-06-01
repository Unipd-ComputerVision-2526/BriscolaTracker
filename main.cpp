#include <iostream>
#include <vector>
#include <tuple>
#include <opencv2/opencv.hpp>
#include "utils.h"

int main() {
    // 1. Test Caricamento Dataset (Gia' verificato)
    std::string datasetPath = "dataset/Briscola_Trentine";
    std::vector<std::tuple<cv::Mat, Suit, int>> dataset = loadDataset(datasetPath);
    std::cout << "Dataset caricato: " << dataset.size() << " carte." << std::endl;

    // 2. Test VideoFrameManager
    // Proviamo con il primo round del game 1
    std::string videoPath = "dataset/game1/game1round1.mp4";
    std::cout << "\nTest: Analisi video " << videoPath << std::endl;

    // Impostiamo un salto di 15 frame e una soglia di 10.0 (piu' alta = meno frame)
    VideoFrameManager vfm(videoPath, 15, 10.0);

    if (!vfm.isOpened()) {
        return -1;
    }

    // Creo una finestra ridimensionabile per vedere tutto il video
    cv::namedWindow("Frame Interessante", cv::WINDOW_NORMAL);
    cv::resizeWindow("Frame Interessante", 800, 600);

    cv::Mat interestingFrame;
    int count = 0;
    
    // In un progetto reale qui chiameremmo Occhi::recognize(interestingFrame)
    while (vfm.getNextInterestingFrame(interestingFrame)) {
        count++;
        std::cout << "Trovato frame interessante numero: " << count << std::endl;
        
        // Per visualizzare (opzionale, utile per debug)
        cv::imshow("Frame Interessante", interestingFrame);
        
        // Aspetta 500ms tra un frame e l'altro per permetterci di vederli
        // Premere un tasto per andare al prossimo o aspettare
        if (cv::waitKey(500) == 27) break; // ESC per uscire
    }

    std::cout << "Totale frame estratti per questo round: " << count << std::endl;
    std::cout << "Fine test video." << std::endl;

    return 0;
}
