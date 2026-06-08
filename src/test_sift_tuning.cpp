#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>

// Funzione di utilità per testare l'estrazione dei keypoints
void testFeatureExtraction(const std::string& imagePath) {
    cv::Mat img = cv::imread(imagePath);
    if (img.empty()) {
        std::cerr << "Errore: Impossibile caricare " << imagePath << std::endl;
        return;
    }

    std::cout << "--- Analisi di: " << imagePath << " ---" << std::endl;

    // 1. SIFT Standard (Default OpenCV)
    cv::Ptr<cv::SIFT> sift_standard = cv::SIFT::create();
    std::vector<cv::KeyPoint> kp_standard;
    cv::Mat desc_standard;
    sift_standard->detectAndCompute(img, cv::noArray(), kp_standard, desc_standard);
    std::cout << "1. SIFT Standard (Default): " << kp_standard.size() << " keypoints." << std::endl;

    // 2. SIFT Tuned (Contrasto ridotto, edge aumentato)
    // contrastThreshold: da 0.04 a 0.02 (cattura feature più deboli)
    // edgeThreshold: da 10 a 15 (accetta forme più tondeggianti/bordi meno netti)
    cv::Ptr<cv::SIFT> sift_tuned = cv::SIFT::create(0, 3, 0.02, 15, 1.6);
    std::vector<cv::KeyPoint> kp_tuned;
    cv::Mat desc_tuned;
    sift_tuned->detectAndCompute(img, cv::noArray(), kp_tuned, desc_tuned);
    std::cout << "2. SIFT Tuned (Contrasto 0.02, Edge 15): " << kp_tuned.size() << " keypoints." << std::endl;

    // 3. Preprocessing CLAHE + SIFT Standard
    cv::Mat lab_img;
    cv::cvtColor(img, lab_img, cv::COLOR_BGR2Lab);
    
    std::vector<cv::Mat> lab_channels;
    cv::split(lab_img, lab_channels);
    
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(3.0);
    clahe->setTilesGridSize(cv::Size(8, 8));
    
    cv::Mat clahe_l_channel;
    clahe->apply(lab_channels[0], clahe_l_channel);
    
    clahe_l_channel.copyTo(lab_channels[0]);
    cv::Mat preprocessed_img;
    cv::merge(lab_channels, preprocessed_img);
    cv::cvtColor(preprocessed_img, preprocessed_img, cv::COLOR_Lab2BGR);

    std::vector<cv::KeyPoint> kp_clahe;
    cv::Mat desc_clahe;
    sift_standard->detectAndCompute(preprocessed_img, cv::noArray(), kp_clahe, desc_clahe);
    std::cout << "3. Preprocessing CLAHE + SIFT Standard: " << kp_clahe.size() << " keypoints." << std::endl;
    std::cout << std::endl;
}

int main() {
    // Testiamo un denari difficile (es. 5 di denari o 10 di denari)
    std::string testImage1 = "../dataset/Briscola_Trentine/5-coins.JPG";
    std::string testImage2 = "../dataset/Briscola_Trentine/10-coins.JPG";
    
    testFeatureExtraction(testImage1);
    testFeatureExtraction(testImage2);

    return 0;
}
