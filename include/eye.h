#ifndef EYE_H
#define EYE_H

#include <vector>
#include "utils.h"

class Eye{
    public:
        // Constructor
        Eye();
        // Object reset
        void clear();
        // Train the object with a set of image, each associated with suit and value
        void fit(std::vector<std::tuple<cv::Mat, Suit, int>> trainingset);
        // Recognize the card in an image, returns suit and value
        std::pair<Suit, int> recognize(cv::Mat image);

    private:
        //Recognized card with suit and value
        std::pair<Suit, int> card;
        // Map that associates image descriptors and cards
        std::map<std::vector<cv::Mat>, std::pair<Suit, int>> cardMap;
        // Already recognized cards
        std::vector<std::pair<Suit, int>> recognizedCards;
        // Empty recognizedCards vector
        void resetRecognizedCards();
};

#endif