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

class VideoFrameManager {
public:
    // Costruttore: apre il video e imposta i parametri di salto frame e soglia movimento
    // motionThreshold: indica quanta differenza deve esserci tra i frame per considerarli diversi (5.0 è un buon punto di partenza)
    VideoFrameManager(const std::string& videoPath, int frameSkip = 10, double motionThreshold = 5.0);
    
    // Distruttore: rilascia le risorse del video
    ~VideoFrameManager();

    // Ritorna true se il video è aperto correttamente
    bool isOpened() const;

    // Riempie 'frame' con il prossimo frame significativo e ritorna false quando il video termina.
    bool getNextInterestingFrame(cv::Mat& frame);

private:
    cv::VideoCapture cap;
    cv::Mat lastInterestingFrameGray;
    int skip;
    double threshold;
    
    // Funzione interna per calcolare la differenza tra due frame (SAD)
    double calculateFrameDifference(const cv::Mat& current, const cv::Mat& last);
};

#endif

