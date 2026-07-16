#ifndef TEMPLATEMATCHER_H
#define TEMPLATEMATCHER_H

#include <opencv2/features2d.hpp>
#include "utils.h"


class TemplateCoinMatcher
{
    public:
        void addTemplates(const cv::Mat& templLarge, const cv::Mat& templMedium);
        void computeRatio(const cv::Mat& coin4, const cv::Mat& coin6);

        bool match(const cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card);
        int circleCounter(const cv::Mat& img, const cv::Mat& mask);

        double calculateAspectRatios(const std::vector<cv::Rect>& rects);
        std::vector<cv::Rect> getCoinRects(const cv::Mat& img, const cv::Mat& mask);

    private:
        cv::Mat templLarge_;
        cv::Mat templMedium_;

        double expectedRatio4_ = 0.0;
        double expectedRatio6_ = 0.0;


};

#endif