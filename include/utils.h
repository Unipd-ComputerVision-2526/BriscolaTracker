#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <tuple>
#include <filesystem>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

/**
 * @brief Card suit enumeration used across the project.
 */
enum Suit{
    /** @brief Coins suit (Denari). */
    COINS=1,
    /** @brief Cups suit (Coppe). */
    CUPS=2,
    /** @brief Swords/Spades suit (Spade). */
    SWORDS=3,
    /** @brief Clubs/Batons suit (Bastoni). */
    CLUBS=4
};

/**
 * @brief Checks whether a filesystem entry is a valid JPG image file.
 * @param entry Filesystem entry to validate.
 * @return true for .JPG/.jpg files, false otherwise.
 */
bool isValidImageFile(const fs::directory_entry& entry);

/**
 * @brief Converts a suit name string to the corresponding Suit enum value.
 * @param semeStr Suit name string (for example "clubs").
 * @return Corresponding Suit value; returns COINS for unknown inputs.
 */
Suit stringToSuit(const std::string& semeStr);

/**
 * @brief Parses card number and suit from a dataset file name.
 * @param filename File name to parse (for example "10-clubs.JPG").
 * @param numero [out] Parsed card number.
 * @param seme [out] Parsed card suit.
 * @return true if parsing succeeds, false if the file name format is invalid.
 */
bool parseFileName(const std::string& filename, int& numero, Suit& seme);

/**
 * @brief Loads dataset card images from the specified directory.
 * @param datasetPath Path to the dataset folder.
 * @return Vector of tuples containing image matrix, suit, and card number
 *         for readable JPG files with a valid card filename format.
 */
std::vector<std::tuple<cv::Mat, Suit, int>> loadDataset(const std::string& datasetPath);

#endif
