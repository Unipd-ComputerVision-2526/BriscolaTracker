#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <map>

using namespace cv;
using namespace std;

struct Detection {
    Rect rect;
    double score;
    double scale;
    int templateIdx; 
};

double getIoU(Rect r1, Rect r2) {
    int interArea = (r1 & r2).area();
    int unionArea = r1.area() + r2.area() - interArea;
    if (unionArea == 0) return 0;
    return (double)interArea / unionArea;
}

int main(int argc, char** argv) {
    // 1. TEMPLATE GRANDE (dal 3 di denari)
    string templateSourcePathLarge = "dataset/Briscola_Trentine/3-coins.JPG";
    Mat templateSourceLarge = imread(templateSourcePathLarge);
    if (templateSourceLarge.empty()) {
        cerr << "Errore: impossibile caricare " << templateSourcePathLarge << endl;
        return -1;
    }
    Rect roiLarge(222, 84, 127, 131);
    Mat templLarge = templateSourceLarge(roiLarge).clone();

    // 2. TEMPLATE MEDIO (dal 7 di denari)
    string templateSourcePath7 = "dataset/Briscola_Trentine/7-coins.JPG";
    Mat templateSource7 = imread(templateSourcePath7);
    if (templateSource7.empty()) {
        cerr << "Errore: impossibile caricare " << templateSourcePath7 << endl;
        return -1;
    }
    Rect roi7(406, 47, 94, 89);
    Mat templ7 = templateSource7(roi7).clone();

    vector<Mat> templates = {templLarge, templ7};
    vector<string> templateNames = {"GRANDE(3)", "MEDIO(7)"};

    // Ground truth FILTRATA (Solo frame SENZA FIGURE: 8, 9, 10 esclusi)
    map<string, int> ground_truth = {
        {"frame4.jpg", 5}, {"frame37.jpg", 2}, {"frame105.jpg", 4}, 
        {"frame144.jpg", 3}, {"frame197.jpg", 7}, {"frame206.jpg", 14}, 
        {"frame214.jpg", 5}, {"frame254.jpg", 2}, {"frame256.jpg", 5}, 
        {"frame317.jpg", 6}, {"frame335.jpg", 0}, {"frame387.jpg", 4}, 
        {"frame424.jpg", 5}, {"frame434.jpg", 6}, {"frame446.jpg", 2}, 
        {"frame466.jpg", 4}, {"frame493.jpg", 3}, {"frame525.jpg", 7}, 
        {"frame584.jpg", 0}, {"frame601.jpg", 4}, {"frame657.jpg", 8}, 
        {"frame705.jpg", 9}, {"frame707.jpg", 11}, {"frame756.jpg", 4}, 
        {"frame766.jpg", 11}, {"frame784.jpg", 4}, {"frame797.jpg", 3}
    };

    double scaleStart = 0.3;
    double scaleEnd = 1.5; 
    double scaleStep = 0.1;
    double thresholdScore = 0.60; 

    cout << "\n--- TEST DOPPIO TEMPLATE (Ottimizzato e Definitivo) ---" << endl;
    int total_frames = 0;
    int perfect_matches = 0;

    for (const auto& pair : ground_truth) {
        string frameName = pair.first;
        int expected = pair.second;
        Mat frame = imread("extracted_samples/" + frameName);
        if (frame.empty()) continue;

        total_frames++;
        auto start_time = chrono::high_resolution_clock::now();
        
        Rect searchROI(frame.cols * 0.1, frame.rows * 0.1, frame.cols * 0.8, frame.rows * 0.8);
        Mat frameROI = frame(searchROI);
        
        vector<Detection> all_detections;

        for (int tIdx = 0; tIdx < templates.size(); tIdx++) {
            Mat t = templates[tIdx];
            for (double scale = scaleStart; scale <= scaleEnd; scale += scaleStep) {
                Mat resizedTempl;
                resize(t, resizedTempl, Size(), scale, scale);

                if (resizedTempl.cols < 38 || resizedTempl.rows < 38 || (resizedTempl.cols * resizedTempl.rows) < 1500) {
                    continue;
                }

                if (resizedTempl.cols > frameROI.cols || resizedTempl.rows > frameROI.rows) continue;

                Mat result;
                matchTemplate(frameROI, resizedTempl, result, TM_CCOEFF_NORMED);
                Mat resultThresh;
                threshold(result, resultThresh, thresholdScore, 1.0, THRESH_TOZERO);

                while (true) {
                    double maxVal; Point maxLoc;
                    minMaxLoc(resultThresh, nullptr, &maxVal, nullptr, &maxLoc);
                    if (maxVal < thresholdScore) break;

                    int globalX = searchROI.x + maxLoc.x;
                    int globalY = searchROI.y + maxLoc.y;
                    all_detections.push_back({Rect(globalX, globalY, resizedTempl.cols, resizedTempl.rows), maxVal, scale, tIdx});

                    int maskRadiusX = max(1, resizedTempl.cols / 2);
                    int maskRadiusY = max(1, resizedTempl.rows / 2);
                    Point pt1(max(0, maxLoc.x - maskRadiusX), max(0, maxLoc.y - maskRadiusY));
                    Point pt2(min(resultThresh.cols - 1, maxLoc.x + maskRadiusX), min(resultThresh.rows - 1, maxLoc.y + maskRadiusY));
                    rectangle(resultThresh, pt1, pt2, Scalar(0), FILLED);
                }
            }
        }

        sort(all_detections.begin(), all_detections.end(), [](const Detection& a, const Detection& b) {
            return a.score > b.score;
        });

        vector<Detection> final_detections;
        for (const auto& det : all_detections) {
            bool keep = true;
            for (const auto& final_det : final_detections) {
                if (getIoU(det.rect, final_det.rect) > 0.2) { 
                    keep = false;
                    break;
                }
            }
            if (keep) final_detections.push_back(det);
        }

        auto end_time = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();

        int found = final_detections.size();
        if (found == expected) perfect_matches++;

        string status = (found == expected) ? "[OK]" : "[ERRORE]";
        cout << status << " " << frameName << " -> Attesi: " << expected << ", Trovati: " << found << " (Tempo: " << duration << " ms)" << endl;
        
        if (found > 0) {
            for (const auto& det : final_detections) {
                cout << "     - " << templateNames[det.templateIdx] 
                     << " [Scala: " << det.scale 
                     << ", Score: " << det.score << "]" << endl;
            }
        }
        cout << endl;
    }

    cout << "\n=== RISULTATI FINALI ===" << endl;
    cout << "Frame perfetti (SENZA FIGURE): " << perfect_matches << " su " << total_frames << endl;

    return 0;
}
