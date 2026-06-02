#include <iostream>
#include <vector>
#include <tuple>
#include <opencv2/opencv.hpp>
#include "utils.h"
#include "video_manager.h"

int main() {
    std::cout << "--- Test Atomicità Moduli ---" << std::endl;

    // 1. Test Utils (Caricamento Dataset)
    std::string datasetPath = "dataset/Briscola_Trentine";
    auto dataset = loadDataset(datasetPath);
    std::cout << "[Utils] Dataset caricato: " << dataset.size() << " carte." << std::endl;

    if (dataset.empty()) {
        std::cerr << "[Errore] Dataset non trovato in " << datasetPath << std::endl;
        return -1;
    }

    // 2. Test VideoFrameManager
    std::string videoPath = "dataset/game1/game1round1.mp4";
    VideoFrameManager vfm(videoPath, 30, 10.0); // Salto 30 frame per velocità

    if (!vfm.isOpened()) {
        std::cerr << "[Errore] Video non trovato: " << videoPath << std::endl;
        return -1;
    }

    cv::Mat frame;
    int count = 0;
    while (vfm.getNextInterestingFrame(frame)) {
        count++;
    }

    std::cout << "[VideoManager] Frame interessanti estratti: " << count << std::endl;
    
    if (count > 0) {
        std::cout << "--- TEST COMPLETATO CON SUCCESSO ---" << std::endl;
    } else {
        std::cout << "--- TEST FALLITO: Nessun frame estratto ---" << std::endl;
    }

    return 0;
}
