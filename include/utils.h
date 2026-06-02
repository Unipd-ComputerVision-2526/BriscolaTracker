#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <tuple>
#include <filesystem>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

enum Suit{
    COINS=1,
    CUPS=2,
    SWORDS=3,
    CLUBS=4
};

// Verifica se il file è un'immagine JPG valida da processare
bool isValidImageFile(const fs::directory_entry& entry);

// Converte una stringa (es. "clubs") nel corrispondente enum Seme
Suit stringToSuit(const std::string& semeStr);

// Estrae Numero e Seme dal nome del file (es: "10-clubs.JPG") e ritorna false se il formato non è corretto.
bool parseFileName(const std::string& filename, int& numero, Suit& seme);

// Legge tutte le immagini delle carte dal percorso specificato.
std::vector<std::tuple<cv::Mat, Suit, int>> loadDataset(const std::string& datasetPath);

#endif
