/**
 * @file templateCoinMatcher.h
 * @brief Header file for the TemplateCoinMatcher class, which matches coin cards using templates.
 * @author Giovanni Stefanuto
 */

#ifndef TEMPLATEMATCHER_H
#define TEMPLATEMATCHER_H

#include <opencv2/features2d.hpp>
#include "utils.h"

/**
 * @brief Class that handles template matching for recognizing "Coins" (Denari) suit cards.
 */
class TemplateCoinMatcher
{
    public:
        /**
         * @brief Adds the large and medium templates used for matching.
         * @param templLarge The large template image.
         * @param templMedium The medium template image.
         */
        void addTemplates(const cv::Mat& templLarge, const cv::Mat& templMedium);

        /**
         * @brief Computes the dynamic aspect ratio for 4 and 6 of coins based on reference images.
         * @param coin4 The reference image for 4 of coins.
         * @param coin6 The reference image for 6 of coins.
         */
        void computeRatio(const cv::Mat& coin4, const cv::Mat& coin6);

        /**
         * @brief Matches the provided image and mask to determine if it is a specific coin card.
         * @param img The input image containing the card.
         * @param mask The motion mask corresponding to the card.
         * @param card [out] The pair that will store the recognized Suit and number if matched.
         * @return true if a match is found, false otherwise.
         */
        bool match(const cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card);

        /**
         * @brief Calculates the aspect ratio for a given set of four rectangles.
         * @param rects A vector of four rectangles.
         * @return The calculated aspect ratio.
         */
        double calculateAspectRatios(const std::vector<cv::Rect>& rects);

        /**
         * @brief Detects and extracts bounding rectangles of individual coins in the image.
         * @param img The input image.
         * @param mask The corresponding mask.
         * @return A vector of bounding rectangles for the detected coins.
         */
        std::vector<cv::Rect> getCoinRects(const cv::Mat& img, const cv::Mat& mask);

    private:
        /** @brief The large template used for matching. */
        cv::Mat templLarge_;
        /** @brief The medium template used for matching. */
        cv::Mat templMedium_;

        /** @brief Expected aspect ratio for the 4 of coins. */
        double expectedRatio4_ = 0.0;
        /** @brief Expected aspect ratio for the 6 of coins (considering the top 4 coins). */
        double expectedRatio6_ = 0.0;
};

#endif