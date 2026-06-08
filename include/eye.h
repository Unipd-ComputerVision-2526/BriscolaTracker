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
        bool recognize(const cv::Mat& image, std::pair<Suit, int>& card);

        bool hasBriscola() {return recognizedBriscola;};
        void setBriscola(std::pair<Suit, int> b) { 
            recognizedCards_.push_back(b); 
            recognizedBriscola = true; 
        }

    private:
        // Feature detector
        cv::Ptr<cv::FastFeatureDetector> fast_;
        // Feature extractor
        cv::Ptr<cv::SIFT> sift_;
        cv::Ptr<cv::ORB> orb_;
        // Feature Matcher
        cv::FlannBasedMatcher matcher_;
        cv::BFMatcher BFmatcher_;
        // Recognized card with suit and value
        std::pair<Suit, int> card_;
        // Map that associates image descriptors and cards
        std::map<std::pair<Suit, int>, std::vector<cv::Mat>> cardMap_;
        // Vector that contains cards values used for training, useful for the matcher
        std::vector<std::pair<Suit, int>> cardVector_;
        // Already recognized cards
        std::vector<std::pair<Suit, int>> recognizedCards_;
        // Last used mask
        cv::Mat lastMask_;

        bool recognizedBriscola=false;

        bool isValidImage(const cv::Mat& img);
        bool validModelState();

        cv::Mat preprocessImage(const cv::Mat& img);

        bool findCardPosition(const cv::Mat& img, cv::Mat& mask);
        bool findCardValue(const cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card);
        bool recognizeBriscola(const cv::Mat& img, std::pair<Suit, int>& card);
        bool recognizeRoundCard(const cv::Mat& img, std::pair<Suit, int>& card);
};

#endif