/**
 * @file video_manager.cpp
 * @author Giovanni Stefanuto
 */


#include "video_manager.h"
#include <iostream>

VideoFrameManager::VideoFrameManager(const std::string& videoPath)
    : skip(10), threshold(5.0), fixedMode(false) {
    cap.open(videoPath, cv::CAP_FFMPEG);
    if (!cap.isOpened()) {
        std::cerr << "Error: Unable to open video stream at: " << videoPath << std::endl;
        return;
    }
    cap.set(cv::CAP_PROP_ORIENTATION_AUTO, 1);
}

VideoFrameManager::VideoFrameManager(const std::string& videoPath, int frameSkip, double motionThreshold) 
    : skip(frameSkip), threshold(motionThreshold), fixedMode(false) {
    cap.open(videoPath, cv::CAP_FFMPEG);
    if (!cap.isOpened()) {
        std::cerr << "Error: Unable to open video stream at: " << videoPath << std::endl;
        return;
    }
    cap.set(cv::CAP_PROP_ORIENTATION_AUTO, 1);
}

VideoFrameManager::VideoFrameManager(const std::string& videoPath, int totalFramesToExtract)
    : fixedMode(true), targetCount(totalFramesToExtract), extractedCount(0) {
    cap.open(videoPath, cv::CAP_FFMPEG);
    if (!cap.isOpened()) {
        std::cerr << "Error: Unable to open video stream at: " << videoPath << std::endl;
        return;
    }
    cap.set(cv::CAP_PROP_ORIENTATION_AUTO, 1);

    double totalFrames = cap.get(cv::CAP_PROP_FRAME_COUNT);
    if (totalFrames > 0 && totalFramesToExtract > 0) {
        frameStep = static_cast<int>(totalFrames) / totalFramesToExtract;
        if (frameStep == 0) frameStep = 1;
    } else {
        frameStep = 1;
    }
}

VideoFrameManager::~VideoFrameManager() {
    if (cap.isOpened()) {
        cap.release();
    }
}

bool VideoFrameManager::isOpened() const {
    return cap.isOpened();
}

double VideoFrameManager::calculateFrameDifference(const cv::Mat& current, const cv::Mat& last) {
    if (current.empty() || last.empty()) return 0.0;

    cv::Mat diff;
    cv::absdiff(current, last, diff);

    // Mean absolute pixel difference used as motion score.
    cv::Scalar meanDiff = cv::mean(diff);
    return meanDiff[0];
}

bool VideoFrameManager::getNextInterestingFrame(cv::Mat& frame) {
    if (!cap.isOpened()) return false;

    if (fixedMode) {
        if (extractedCount >= targetCount) return false;

        // Jump directly to the next scheduled sample index.
        int targetPos = extractedCount * frameStep;
        cap.set(cv::CAP_PROP_POS_FRAMES, targetPos);

        if (cap.read(frame)) {
            double rotation = cap.get(cv::CAP_PROP_ORIENTATION_META) - 90.0;

            if (rotation == 90.0) {
                cv::rotate(frame, frame, cv::ROTATE_90_CLOCKWISE);
            } else if (rotation == 180.0) {
                cv::rotate(frame, frame, cv::ROTATE_180);
            } else if (rotation == 270.0) {
                cv::rotate(frame, frame, cv::ROTATE_180);
            }

            extractedCount++;
            return true;
        }
        return false;
    }

    cv::Mat currentFrame;
    while (cap.read(currentFrame)) {
        cv::Mat graySmall;
        double rotation = cap.get(cv::CAP_PROP_ORIENTATION_META) - 90.0;

        if (rotation == 90.0) {
            cv::rotate(currentFrame, currentFrame, cv::ROTATE_90_CLOCKWISE);
        } else if (rotation == 180.0) {
            cv::rotate(currentFrame, currentFrame, cv::ROTATE_180);
        } else if (rotation == 270.0) {
            cv::rotate(currentFrame, currentFrame, cv::ROTATE_90_COUNTERCLOCKWISE);
        }

        cv::cvtColor(currentFrame, graySmall, cv::COLOR_BGR2GRAY);
        cv::resize(graySmall, graySmall, cv::Size(128, 128));

        if (lastInterestingFrameGray.empty()) {
            currentFrame.copyTo(frame);
            graySmall.copyTo(lastInterestingFrameGray);
            return true;
        }

        double diff = calculateFrameDifference(graySmall, lastInterestingFrameGray);

        if (diff > threshold) {
            currentFrame.copyTo(frame);
            graySmall.copyTo(lastInterestingFrameGray);

            // Skip a short window to avoid near-duplicate frames from one motion burst.
            for (int i = 0; i < skip; ++i) {
                cap.grab();
            }

            return true;
        }
    }

    return false;
}
