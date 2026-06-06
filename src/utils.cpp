#include <utils.h>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

// Verifica se il file è un'immagine JPG valida da processare
bool isValidImageFile(const fs::directory_entry& entry) {
    std::string ext = entry.path().extension().string();

    // Salto i file che non sono immagini JPG o file di sistema tipo Zone.Identifier
    return (ext == ".JPG" || ext == ".jpg");
}

// Converte una stringa nel corrispondente enum Suit
Suit stringToSuit(const std::string& semeStr) {
    if (semeStr == "coins") return COINS;
    if (semeStr == "cups") return CUPS;
    if (semeStr == "swords" || semeStr == "spades") return SWORDS;
    if (semeStr == "clubs") return CLUBS;
    
    std::cerr << "Errore: Suit sconosciuto '" << semeStr << "'. Fallback a COINS." << std::endl;
    return COINS;
}

// Estrae Numero e Suit dal nome del file (es: "10-clubs.JPG")
bool parseFileName(const std::string& filename, int& numero, Suit& seme) {
    size_t posDash = filename.find('-'); //posizione del trattino
    size_t posDot = filename.find('.'); //posizione del punto dell'estensione 
    
    if (posDash == std::string::npos || posDot == std::string::npos) {
        return false; //npos è un valore speciale che indica "non trovato", se non troviamo il trattino o il punto, il formato è errato.
    }

    try {
        std::string numStr = filename.substr(0, posDash); //estrai la parte del numero prima del trattino
        std::string semeStr = filename.substr(posDash + 1, posDot - posDash - 1); //estrai la parte del seme tra il trattino e il punto
        
        numero = std::stoi(numStr); //converte la stringa del numero in un intero.
        seme = stringToSuit(semeStr); //converte la stringa del seme in un enum Suit.
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// Legge tutte le immagini delle carte dal percorso specificato.
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