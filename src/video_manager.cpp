#include "video_manager.h"
#include <iostream>

VideoFrameManager::VideoFrameManager(const std::string& videoPath, int frameSkip, double motionThreshold) 
    : cap(videoPath), skip(frameSkip), threshold(motionThreshold) {
    if (!cap.isOpened()) {
        std::cerr << "Errore: Impossibile aprire il video: " << videoPath << std::endl;
    }
}

VideoFrameManager::~VideoFrameManager() {
    if (cap.isOpened()) {
        cap.release();
    }
}

bool VideoFrameManager::isOpened() const {
    return cap.isOpened();
}

double VideoFrameManager::calculateFrameDifference(const cv::Mat& current, const cv::Mat& last) {
    if (current.empty() || last.empty()) return 0.0;

    cv::Mat diff;
    cv::absdiff(current, last, diff);
    
    // Calcola il valore medio della differenza
    cv::Scalar meanDiff = cv::mean(diff);
    return meanDiff[0];
}

bool VideoFrameManager::getNextInterestingFrame(cv::Mat& frame) {
    if (!cap.isOpened()) return false;

    cv::Mat currentFrame;
    while (cap.read(currentFrame)) {
        // Converto in scala di grigi e riduco la risoluzione per velocità nel calcolo differenza
        cv::Mat graySmall;
        cv::cvtColor(currentFrame, graySmall, cv::COLOR_BGR2GRAY);
        cv::resize(graySmall, graySmall, cv::Size(128, 128));

        // Se è il primo frame della storia, lo consideriamo sempre "interessante"
        if (lastInterestingFrameGray.empty()) {
            currentFrame.copyTo(frame);
            graySmall.copyTo(lastInterestingFrameGray);
            return true;
        }

        // Calcolo la differenza con l'ultimo frame che abbiamo considerato interessante
        double diff = calculateFrameDifference(graySmall, lastInterestingFrameGray);

        // Se la differenza supera la soglia, abbiamo trovato un frame interessante
        if (diff > threshold) {
            currentFrame.copyTo(frame);
            graySmall.copyTo(lastInterestingFrameGray);
            
            // Salto N frame per evitare di catturare troppi frame durante lo stesso movimento
            for (int i = 0; i < skip; ++i) {
                cap.grab(); // grab() è più veloce di read() perché non decodifica l'immagine
            }
            
            return true;
        }
    }

    return false;
}
