/**
 * @file eye.h
 * @brief Definition of the Eye class for the visual model that recognizes the cards and the associated players.
 * @author Daniele Daccordo
 */


#ifndef EYE_H
#define EYE_H

#include <opencv2/features2d.hpp>
#include <vector>
#include "utils.h"
#include "templateCoinMatcher.h"

/**
 * @class Eye
 * @brief Computer Vision system that analyzes frames and recognizes player activity and cards from a dataset.
 * 
 * Given a provided dataset, this class is responsible for analyzing video frames using OpenCV feature
 * extractors (SIFT, AKAZE) and matchers, recognizing the Briscola cards present in it. 
 * It also tracks frame-to-frame motion to determine which player (Nord/Sud) played the recognized card.
 */

class Eye
{
    public:
        /**
         * @brief Initializes the vision system, extractors, and matchers.
         * 
         * Creates instances of SIFT and AKAZE, initializes FlannBased and Brute-Force matchers,
         * and sets default motion tracking states.
         */
        Eye();

        /**
         * @brief Clears the current tracking state for the active frame.
         * 
         * Resets the residual image, last used mask, and zeroes out the motion accumulators.
         * Does not erase the trained model dataset.
         */
        void clear();

        /**
         * @brief Performs a hard reset of the model.
         * 
         * Clears the current frame matching state and empties the trained card dataset,
         * requiring a new call to fit() before recognizing again.
         */
        void reset();

        /**
         * @brief Trains the feature matching model with a labeled dataset of cards.
         * 
         * Extracts SIFT and AKAZE descriptors from the provided images and sets up template
         * matchers for edge cases (like the Coins suit). It works only if the dataset 
         * represents a complete 40-card deck.
         * 
         * @param trainingset A vector of tuples, each containing the image (cv::Mat), its Suit, and its integer value.
         * @throws std::invalid_argument If an image format is invalid or if the dataset does not form a valid Briscola deck.
         */
        void fit(const std::vector<std::tuple<cv::Mat, Suit, int>>& trainingset);
        
        /**
         * @brief Analyzes a frame to recognize a card and track player motion.
         * 
         * @param image The input video frame (cv::Mat) to analyze.
         * @param card Reference to a pair (Suit, int) that will be populated with the recognized card.
         * If no new card is found, it will be populated with the most probable card or the last recognized one 
         * if no region of interest is found.
         * @returns true if a new card is successfully recognized in the current frame, false otherwise.
         * @throws std::logic_error If called before the model is trained with fit().
         * @throws std::invalid_argument If the input image format is invalid.
         */
        bool recognize(const cv::Mat& image, std::pair<Suit, int>& card);

        /**
         * @brief Checks if the "Nord" (top) player played the last recognized card.
         * 
         * @returns true if accumulated motion in the top zone exceeded the bottom zone, false otherwise.
         */
        bool wasNordActive() {return plN_;};

        /**
         * @brief Retrieves the most recently recognized card from the active state.
         * 
         * @param card Reference to a pair (Suit, int) that will be populated with the last recognized card.
         * @returns true if a previous card exists in the active state, false otherwise.
         */
        bool getLastCard(std::pair<Suit, int>& card);
        
    private:
        /** @brief SIFT feature extractor instance. */
        cv::Ptr<cv::SIFT> sift_;
        /** @brief AKAZE feature extractor instance. */
        cv::Ptr<cv::AKAZE> akaze_;

        /** @brief Flann Based matcher used for SIFT descriptors. */
        cv::FlannBasedMatcher fl_matcher_;
        /** @brief Brute-Force matcher utilizing Hamming distance for AKAZE descriptors. */
        cv::BFMatcher bf_matcher_;
        /** @brief Custom matcher to handle the specific visual properties of the Coins suit in a template matching manner. */
        TemplateCoinMatcher tc_matcher_;

        /** @brief The suit and value of the most recently recognized card. */
        std::pair<Suit, int> card_;

        /** @brief Dataset array storing the suit and value of all cards loaded during fit(). */
        std::vector<std::pair<Suit, int>> cardVector_;

        /** @brief The mask defining the card position in the previously processed frame. */
        cv::Mat lastMask_;

        /** @brief The original image of the previously processed frame, used for motion differencing. */
        cv::Mat residualImage_;
        
        /** @brief Flag indicating if the "Nord" (top) player played the last card. */
        bool plN_ = false;

        /** @brief Accumulated pixel motion count in the top 40% of the frame. */
        int accumulatedUp_ = 0;
        /** @brief Accumulated pixel motion count in the bottom 40% of the frame. */
        int accumulatedBot_ = 0;

        /**
         * @brief Validates the matrix dimensions, depth, and channels of the input image.
         * @param img The image matrix to check.
         * @returns true if the image is valid for processing, false otherwise.
         */
        bool isValidImage(const cv::Mat& img);
        /**
         * @brief Validates that the loaded dataset contains exactly 40 unique cards of a standard deck.
         * @returns true if the model state is valid, false otherwise.
         */
        bool isValidModelState();
        
        /**
         * @brief Enhances the image for feature extraction (e.g., applying CLAHE on the Lab color space and shifting the mean of the "b" channel).
         * @param img The source image.
         * @param dst The destination matrix for the preprocessed image.
         */
        void preprocessImage(const cv::Mat& img, cv::Mat& dst);
        /**
         * @brief Refines the card mask using frame differencing against the residual image.
         * @param img The current frame.
         * @param mask The initial position mask.
         * @param dst The destination matrix for the refined difference mask.
         */
        void processMask(const cv::Mat& img, const cv::Mat& mask, cv::Mat& dst);
        /**
         * @brief Accumulates pixel changes in the top and bottom zones to determine player activity.
         * @param img The current frame.
         * @param flush If true, evaluates the accumulated motion to set the active player flag.
         */
        void accumulateMotion(const cv::Mat& img, bool flush);
        
        /**
         * @brief Isolates the card from the background using HSV Z-score thresholding.
         * @param img The downscaled input image.
         * @param mask_out The output binary mask indicating card position.
         * @returns true if a card-like shape of sufficient size is found, false otherwise.
         */
        bool findCardPosition(const cv::Mat& img, cv::Mat& mask_out);
        /**
         * @brief Determines the suit and value by chaining SIFT, AKAZE, and template matching.
         * @param img The original frame.
         * @param mask The refined difference mask.
         * @param card Reference pair to populate upon successful recognition.
         * @returns true if the card is successfully identified, false otherwise.
         */
        bool findCardValue(const cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card);
        /**
         * @brief Master orchestration function for scaling, masking, and value recognition.
         * @param img The original frame.
         * @param card Reference pair to populate upon successful recognition.
         * @param diffMask Reference to output the computed difference mask for motion tracking.
         * @returns true if a card is successfully processed and recognized.
         */
        bool recognizeCard(const cv::Mat& img, std::pair<Suit, int>& card, cv::Mat& diffMask);

        /**
         * @brief Attempts card recognition using SIFT descriptors and KNN matching.
         * @param img The input image.
         * @param mask The region of interest mask.
         * @param card Reference pair to populate on match.
         * @returns true if at least 15 valid keypoints match a single card.
         */
        bool siftRecognition(cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card);
        /**
         * @brief Attempts card recognition using AKAZE descriptors on Lab channels.
         * @param img The input image.
         * @param mask The region of interest mask.
         * @param card Reference pair to populate on match.
         * @returns true if at least 10 valid keypoints match a single card.
         */
        bool akazeRecognition(cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card);
};

#endif