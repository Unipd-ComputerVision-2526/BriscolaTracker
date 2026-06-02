#ifndef EYE_H
#define EYE_H

#include <opencv2/features2d.hpp>
#include <vector>
#include "utils.h"

class Eye
{
    public:
        // Constructor
        Eye();
        // Object reset
        void clear();
        // Train the object with a set of image, each associated with suit and value
        void fit(const std::vector<std::tuple<cv::Mat, Suit, int>>& trainingset);
        // Recognize the card in an image, returns suit and value in the argument pair
        void recognize(const cv::Mat& image, std::pair<Suit, int>& card);

    private:
        // Feature detector
        cv::Ptr<cv::FastFeatureDetector> fast_;
        // Recognized card with suit and value
        std::pair<Suit, int> card_;
        // Map that associates image descriptors and cards
        std::map<std::pair<Suit, int>, std::vector<cv::KeyPoint>> cardMap_;
        // Already recognized cards
        std::vector<std::pair<Suit, int>> recognizedCards_;

        bool isValidImage(const cv::Mat& img);
        bool validModelState();

        void recognizeBriscola(const cv::Mat& img, std::pair<Suit, int>& card);
        void recognizeRoundCard(const cv::Mat& img, std::pair<Suit, int>& card);
};

#endif