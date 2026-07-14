#include <iostream>
#include <vector>
#include <tuple>
#include <string>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include "utils.h"
#include "eye.h"
#include "gameManager.h" 

int main() {
    //Define paths for the dataset and the game videos
    std::string datasetPath = "../dataset/Briscola_Trentine";
    std::string baseFolderPath = "../dataset/";

    //Load the dataset of reference cards (the template images for the vision system)
    std::vector<std::tuple<cv::Mat, Suit, int>> dataset = loadDataset(datasetPath);
    if (dataset.empty()) {
        std::cerr << "Error: Dataset not found in " << datasetPath << std::endl;
        return -1;
    }
    std::cout << "Dataset loaded successfully: " << dataset.size() << " cards." << std::endl;

    //Initialize and train the computer vision system (Eye)
    Eye watcher;
    watcher.fit(dataset);

    //Initialize the game manager, passing the trained vision system
    GameManager manager(&watcher);

    int g = 1;
    while (true) {
        std::string gameName = "game" + std::to_string(g);
        std::string expectedPath = baseFolderPath + gameName;

        if (!std::filesystem::exists(expectedPath) || !std::filesystem::is_directory(expectedPath)) {
            break; 
        }

        // Starts the full 20-round analysis for the current game
        manager.processFullGame(gameName, baseFolderPath);
        
        g++; 
    }

    std::cout << "\n>>> ALL THE GAMES HAVE BEEN PROCESSED SUCCESSFULLY!" << std::endl;

    return 0;
}