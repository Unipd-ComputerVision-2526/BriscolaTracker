#ifndef VIDEO_MANAGER_H
#define VIDEO_MANAGER_H

#include <string>
#include <opencv2/opencv.hpp>

/**
 * @brief Handles frame extraction from a video stream.
 *
 * The manager supports two extraction strategies:
 * - motion-based selection of significant frames
 * - fixed-count sampling with approximately uniform frame spacing
 */
class VideoFrameManager {
public:
    /**
     * @brief Creates a manager using default motion-detection parameters.
     * @param videoPath Path to the input video file.
     *
     * Uses internal defaults for frame skipping and motion threshold.
     */
    explicit VideoFrameManager(const std::string& videoPath);

    /**
     * @brief Creates a manager configured for motion-based frame extraction.
     * @param videoPath Path to the input video file.
     * @param frameSkip Number of frames to skip between motion checks.
     * @param motionThreshold Minimum difference threshold to mark a frame as interesting.
     */
    VideoFrameManager(const std::string& videoPath, int frameSkip, double motionThreshold);
    
    /**
     * @brief Creates a manager configured for fixed-count frame sampling.
     * @param videoPath Path to the input video file.
     * @param totalFramesToExtract Number of frames to extract uniformly.
     */
    VideoFrameManager(const std::string& videoPath, int totalFramesToExtract);

    /**
     * @brief Releases resources associated with the video capture.
     */
    ~VideoFrameManager();

    /**
     * @brief Checks whether the video stream was opened successfully.
     * @return true if the capture is open, false otherwise.
     */
    bool isOpened() const;

    /**
     * @brief Retrieves the next frame selected by the active extraction mode.
     * @param frame [out] Output frame buffer.
     * @return true if a valid frame was produced, false when the stream ends or on failure.
     */
    bool getNextInterestingFrame(cv::Mat& frame);

private:
    /** @brief OpenCV capture object used to read frames from the video. */
    cv::VideoCapture cap;
    /** @brief Grayscale representation of the last accepted interesting frame. */
    cv::Mat lastInterestingFrameGray;
    /** @brief Number of frames to skip between motion evaluations. */
    int skip;
    /** @brief Motion difference threshold used in motion-based mode. */
    double threshold;

    /** @brief Enables fixed-count uniform sampling mode when true. */
    bool fixedMode = false;
    /** @brief Target number of frames to extract in fixed mode. */
    int targetCount = 0;
    /** @brief Number of frames already extracted in fixed mode. */
    int extractedCount = 0;
    /** @brief Frame interval used to distribute extracted frames uniformly. */
    int frameStep = 0;
    
    /**
     * @brief Computes frame difference from absolute pixel-wise differences.
     * @param current Current frame (typically grayscale).
     * @param last Previously accepted reference frame.
     * @return Mean absolute difference score used for motion thresholding.
     */
    double calculateFrameDifference(const cv::Mat& current, const cv::Mat& last);
};

#endif
