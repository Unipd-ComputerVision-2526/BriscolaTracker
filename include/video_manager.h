#ifndef VIDEO_MANAGER_H
#define VIDEO_MANAGER_H

#include <string>
#include <opencv2/opencv.hpp>

class VideoFrameManager {
public:
    // Costruttore per default motion detection (usa 10 e 5.0)
    explicit VideoFrameManager(const std::string& videoPath);

    // Costruttore esplicito per motion detection
    VideoFrameManager(const std::string& videoPath, int frameSkip, double motionThreshold);
    
    // Costruttore per campionamento fisso di N frame
    VideoFrameManager(const std::string& videoPath, int totalFramesToExtract);

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

    // Variabili per campionamento fisso
    bool fixedMode = false;
    int targetCount = 0;
    int extractedCount = 0;
    int frameStep = 0;
    
    // Funzione interna per calcolare la differenza tra due frame (SAD)
    double calculateFrameDifference(const cv::Mat& current, const cv::Mat& last);
};

#endif
