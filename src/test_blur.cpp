#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

// Funzione per calcolare la varianza del Laplaciano (ottimizzata senza visualizzazione)
double getLaplacianVariance(const cv::Mat& frame) {
    cv::Mat gray, laplacian, mean, stddev;
    if (frame.channels() == 3) {
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = frame;
    }
    cv::Laplacian(gray, laplacian, CV_64F);
    cv::meanStdDev(laplacian, mean, stddev);
    double stddev_val = stddev.at<double>(0, 0);
    return stddev_val * stddev_val;
}

int main() {
    // Lista delle cartelle da analizzare sequentialmente
    std::vector<std::string> target_folders = {
        "dataset/game1", 
        "dataset/game2", 
        "dataset/game3", 
        "dataset/game4"
    };

    // Timer globale per l'intero dataset
    auto total_start = std::chrono::high_resolution_clock::now();
    
    int total_videos_processed = 0;
    int total_frames_processed = 0;

    std::cout << "==================================================" << std::endl;
    std::cout << "      AVVIO BENCHMARK ANALISI MOTION BLUR        " << std::endl;
    std::cout << "==================================================" << std::endl;

    for (const auto& folder_path : target_folders) {
        if (!fs::exists(folder_path)) {
            std::cout << "\n[INFO] Cartella non trovata: " << folder_path << " -> Salto." << std::endl;
            continue;
        }

        std::cout << "\nScansione della cartella: " << folder_path << std::endl;

        // Iterazione automatica sui file della cartella
        for (const auto& entry : fs::directory_iterator(folder_path)) {
            // Controlla che sia un file regolare e che l'estensione sia .mp4
            if (entry.is_regular_file() && entry.path().extension() == ".mp4") {
                std::string videoPath = entry.path().string();
                std::cout << "--------------------------------------------------" << std::endl;
                std::cout << "Analisi file: " << entry.path().filename().string() << std::endl;

                cv::VideoCapture cap(videoPath);
                if (!cap.isOpened()) {
                    std::cerr << "  [ERRORE] Impossibile aprire il file video." << std::endl;
                    continue;
                }

                // Timer per il singolo video
                auto video_start = std::chrono::high_resolution_clock::now();
                
                std::vector<double> variances;
                cv::Mat frame;

                // Estrazione feature (Fase critica di calcolo)
                while (cap.read(frame)) {
                    variances.push_back(getLaplacianVariance(frame));
                }
                cap.release();

                auto video_end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> video_duration = video_end - video_start;

                if (variances.empty()) {
                    std::cout << "  [AVVISO] Nessun frame estratto da questo video." << std::endl;
                    continue;
                }

                // Calcolo statistico sul "cosa rimane" usando la soglia 0.85
                std::vector<double> sorted_variances = variances;
                std::sort(sorted_variances.begin(), sorted_variances.end());
                
                // Indica il valore di varianza sotto cui si trova l'85% dei frame
                double threshold = sorted_variances[sorted_variances.size() * 0.85];
                
                int sharp_frames_count = 0;
                for (double v : variances) {
                    if (v >= threshold) {
                        sharp_frames_count++;
                    }
                }

                // Output metriche del singolo video
                std::cout << "  -> Elab. completata in : " << video_duration.count() << " secondi" << std::endl;
                std::cout << "  -> Frame totali        : " << variances.size() << std::endl;
                std::cout << "  -> Velocita' parziale  : " << variances.size() / video_duration.count() << " FPS" << std::endl;
                std::cout << "  -> Valore Soglia (0.85): " << threshold << std::endl;
                std::cout << "  -> Frame Nitidi Rimasti: " << sharp_frames_count << " (top 15%)" << std::endl;

                total_frames_processed += variances.size();
                total_videos_processed++;
            }
        }
    }

    // Stop del timer globale
    auto total_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> total_duration = total_end - total_start;

    // Report Finale dei Tempi
    std::cout << "\n==================================================" << std::endl;
    std::cout << "                REPORT FINALE                    " << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Video totali analizzati : " << total_videos_processed << std::endl;
    std::cout << "Frame totali elaborati  : " << total_frames_processed << std::endl;
    std::cout << "TEMPO TOTALE IMPIEGATO  : " << total_duration.count() << " secondi" << std::endl;
    
    if (total_duration.count() > 0) {
        std::cout << "VELOCITA' MEDIA DATASET : " << total_frames_processed / total_duration.count() << " FPS" << std::endl;
    }
    std::cout << "==================================================" << std::endl;

    return 0;
}