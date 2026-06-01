#ifndef VIDEO_MANAGER_H
#define VIDEO_MANAGER_H

#include <string>
#include <opencv2/opencv.hpp>

class VideoFrameManager {
public:
    // Costruttore: apre il video e imposta i parametri di salto frame e soglia movimento
    // motionThreshold: indica quanta differenza deve esserci tra i frame per considerarli diversi
    VideoFrameManager(const std::string& videoPath, int frameSkip = 10, double motionThreshold = 5.0);
    
    // Distruttore: rilascia le risorse del video
    ~VideoFrameManager();

    // Ritorna true se il video è aperto correttamente
    bool isOpened() const;

    // Riempie 'frame' con il prossimo frame significativo e ritorna false quando il video termina.
    bool getNextInterestingFrame(cv::Mat& frame);

private:
    cv::VideoCapture cap;
    cv::Mat lastInterestingFrameGray;
    int skip;
    double threshold;
    
    // Funzione interna per calcolare la differenza tra due frame (SAD)
    double calculateFrameDifference(const cv::Mat& current, const cv::Mat& last);
};

#endif
