#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <map>

namespace fs = std::filesystem;

/**
 * Interactive Color Picker for training color ranges.
 * Controls:
 * - Left Click: Pick color
 * - 'c': Set mode to CARD
 * - 't': Set mode to TABLE
 * - 'm': Set mode to HAND
 * - Space: Next Frame
 * - 'q': Quit and print results
 */

enum class Mode { CARD, TABLE, HAND };
std::map<Mode, std::string> modeNames = {{Mode::CARD, "CARD"}, {Mode::TABLE, "TABLE"}, {Mode::HAND, "HAND"}};

struct PickedColor {
    cv::Vec3b bgr;
    Mode mode;
    int frameIdx;
};

Mode currentMode = Mode::CARD;
std::vector<PickedColor> pickedColors;
cv::Mat currentFrame;
int frameCount = 0;

void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        cv::Vec3b bgr = currentFrame.at<cv::Vec3b>(y, x);
        pickedColors.push_back({bgr, currentMode, frameCount});
        
        std::cout << "[" << modeNames[currentMode] << "] Picked BGR: " 
                  << (int)bgr[0] << ", " << (int)bgr[1] << ", " << (int)bgr[2] 
                  << " at (" << x << "," << y << ")" << std::endl;
        
        // Visual feedback
        cv::circle(currentFrame, cv::Point(x, y), 5, cv::Scalar(0, 255, 0), -1);
        cv::imshow("Color Picker", currentFrame);
    }
}

int main() {
    std::string path = "extracted_samples";
    if (!fs::exists(path)) path = "BriscolaTracker/extracted_samples";
    if (!fs::exists(path)) {
        std::cerr << "Error: Directory extracted_samples not found!" << std::endl;
        return -1;
    }

    std::vector<std::string> files;
    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.path().extension() == ".jpg" || entry.path().extension() == ".JPG") {
            files.push_back(entry.path().string());
        }
    }
    std::sort(files.begin(), files.end());

    cv::namedWindow("Color Picker", cv::WINDOW_NORMAL);
    cv::resizeWindow("Color Picker", 1280, 720);
    cv::setMouseCallback("Color Picker", onMouse);

    std::cout << "--- COLOR PICKER TOOL ---" << std::endl;
    std::cout << "1. Use 'c', 't', 'm' to switch mode (CARD, TABLE, HAND)" << std::endl;
    std::cout << "2. Left Click to sample color" << std::endl;
    std::cout << "3. Space to next frame, 'q' to quit and see summary" << std::endl;

    for (const auto& f : files) {
        currentFrame = cv::imread(f);
        if (currentFrame.empty()) continue;
        frameCount++;

        while (true) {
            cv::Mat display = currentFrame.clone();
            std::string status = "MODE: " + modeNames[currentMode] + " | Frame: " + std::to_string(frameCount);
            cv::putText(display, status, cv::Point(20, 40), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 255), 2);
            
            cv::imshow("Color Picker", display);
            char key = (char)cv::waitKey(0);

            if (key == ' ') break; // Next frame
            if (key == 'q') goto end; // Quit
            if (key == 'c') currentMode = Mode::CARD;
            if (key == 't') currentMode = Mode::TABLE;
            if (key == 'm') currentMode = Mode::HAND;
        }
    }

end:
    std::cout << "\n--- SUMMARY OF PICKED COLORS ---" << std::endl;
    for (const auto& pc : pickedColors) {
        std::cout << modeNames[pc.mode] << "," << (int)pc.bgr[0] << "," << (int)pc.bgr[1] << "," << (int)pc.bgr[2] << std::endl;
    }

    cv::destroyAllWindows();
    return 0;
}
