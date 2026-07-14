#ifndef EYE_H
#define EYE_H

#include <opencv2/features2d.hpp>
#include <vector>
#include "utils.h"

class Eye
{
    public:
        // Constructor
        Eye(int channelNum=3);
        // Clear match
        void clear();
        // Clear also trained model
        void reset();
        // Train the object with a set of image, each associated with suit and value
        void fit(const std::vector<std::tuple<cv::Mat, Suit, int>>& trainingset);
        // Recognize the card in an image, returns suit and value in the argument pair
        bool recognize(const cv::Mat& image, std::pair<Suit, int>& card);
        // Returns if the last  who played was Nord
        bool wasNordActive() {return plN_;};
        // Fills card with the last recognized card if present, else returns false
        bool getLastCard(std::pair<Suit, int>& card);
        
        private:
        // Feature extractor
        cv::Ptr<cv::SIFT> sift_;
        // Feature Matcher
        cv::FlannBasedMatcher matcher_;
        // Recognized card with suit and value
        std::pair<Suit, int> card_;
        // Vector that contains cards values used for training, useful for the matcher
        std::vector<std::pair<Suit, int>> cardVector_;
        // Already recognized cards
        std::vector<std::pair<Suit, int>> recognizedCards_;
        // Last used mask
        cv::Mat lastMask_;
        // Last seen frame;
        cv::Mat residualImage_;
        
        int channelNum_;
        bool plN_;

        bool isValidImage(const cv::Mat& img);
        bool isValidModelState();
        
        void preprocessImage(const cv::Mat& img, cv::Mat& dst);
        void processMask(const cv::Mat& img, const cv::Mat& mask, cv::Mat& dst);
        
        bool findCardPosition(const cv::Mat& img, cv::Mat& mask_out);
        bool findCardValue(const cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card);
        bool recognizeCard(const cv::Mat& img, std::pair<Suit, int>& card, cv::Mat& diffMask);
        bool isNordPlaying(const cv::Mat& img);
};

#endif