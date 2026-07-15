/**
 * @file utils.cpp
 * @author Giovanni Stefanuto
 */


#include <utils.h>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

// Accept only JPG dataset assets.
bool isValidImageFile(const fs::directory_entry& entry) {
    std::string ext = entry.path().extension().string();
    return (ext == ".JPG" || ext == ".jpg");
}

Suit stringToSuit(const std::string& semeStr) {
    if (semeStr == "coins") return COINS;
    if (semeStr == "cups") return CUPS;
    if (semeStr == "swords" || semeStr == "spades") return SWORDS;
    if (semeStr == "clubs") return CLUBS;
    
    std::cerr << "Errore: Suit sconosciuto '" << semeStr << "'. Fallback a COINS." << std::endl;
    return COINS;
}

// Expected format: <number>-<suit>.<ext>
bool parseFileName(const std::string& filename, int& numero, Suit& seme) {
    size_t posDash = filename.find('-');
    size_t posDot = filename.find('.');
    
    if (posDash == std::string::npos || posDot == std::string::npos) {
        return false;
    }

    try {
        std::string numStr = filename.substr(0, posDash);
        std::string semeStr = filename.substr(posDash + 1, posDot - posDash - 1);
        
        numero = std::stoi(numStr);
        seme = stringToSuit(semeStr);
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

std::vector<std::tuple<cv::Mat, Suit, int>> loadDataset(const std::string& datasetPath) {
    std::vector<std::tuple<cv::Mat, Suit, int>> dataset;

    if (!fs::exists(datasetPath)) {
        std::cerr << "Errore: Cartella dataset non trovata: " << datasetPath << std::endl;
        return dataset;
    }

    std::vector<std::string> filePaths;
    for (const auto& entry : fs::directory_iterator(datasetPath)) {
        if (isValidImageFile(entry)) {
            filePaths.push_back(entry.path().string());
        }
    }
    std::sort(filePaths.begin(), filePaths.end());

    for (const std::string& filePath : filePaths) {
        fs::path p(filePath);
        int numero = 0;
        Suit seme = COINS;
        std::string filename = p.filename().string();

        if (parseFileName(filename, numero, seme)) {
            cv::Mat img = cv::imread(filePath);
            if (!img.empty()) {
                dataset.push_back(std::make_tuple(img, seme, numero));
            } else {
                std::cerr << "Errore lettura immagine: " << filename << std::endl;
            }
        }
    }

    return dataset;
}