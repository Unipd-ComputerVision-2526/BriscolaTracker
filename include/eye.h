#ifndef EYE_H
#define EYE_H

#include <opencv2/features2d.hpp>
#include <vector>
#include "utils.h"
#include "templateCoinMatcher.h"

class Eye
{
    public:
        // Constructor
        Eye();
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
        void setBaseline(const cv::Mat& img);
        
    private:
        // Feature extractors
        cv::Ptr<cv::SIFT> sift_;
        cv::Ptr<cv::AKAZE> akaze_;
        // Feature matchers
        cv::FlannBasedMatcher fl_matcher_;
        cv::BFMatcher bf_matcher_;
        TemplateCoinMatcher tc_matcher_;
        // Recognized card with suit and value
        std::pair<Suit, int> card_;
        // Vector that contains cards values used for training, useful for the matcher
        std::vector<std::pair<Suit, int>> cardVector_;
        // Last used mask
        cv::Mat lastMask_;
        // Last seen frame;
        cv::Mat residualImage_;
        cv::Mat coin4, coin6;
        
        bool plN_ = false;
        bool templatesLoaded_ = false;
        double expectedRatio4_ = 0.0;
        double expectedRatio6_ = 0.0;
        int accumulatedUp_ = 0;
        int accumulatedBot_ = 0;

        bool isValidImage(const cv::Mat& img);
        bool isValidModelState();
        
        void preprocessImage(const cv::Mat& img, cv::Mat& dst);
        void processMask(const cv::Mat& img, const cv::Mat& mask, cv::Mat& dst);
        void accumulateMotion(const cv::Mat& img, bool reset);
        void computeCoinRatios();
        void loadTemplates();
        
        bool findCardPosition(const cv::Mat& img, cv::Mat& mask_out);
        bool findCardValue(const cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card);
        bool recognizeCard(const cv::Mat& img, std::pair<Suit, int>& card, cv::Mat& diffMask);

        bool siftRecognition(cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card);
        bool akazeRecognition(cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card);

        int circleCounter(const cv::Mat& img, const cv::Mat& mask);
        double calculateAspectRatios(const std::vector<cv::Rect>& rects);
        std::vector<cv::Rect> getCoinRects(const cv::Mat& img, const cv::Mat& mask);

};

#endif