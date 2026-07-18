/**
 * @file templateCoinMatcher.cpp
 * @author Giovanni Stefanuto
 */

#include "templateCoinMatcher.h"
#include "utils.h"
#include <opencv2/imgproc.hpp>
#include <iostream>

void TemplateCoinMatcher::addTemplates(const cv::Mat& templLarge, const cv::Mat& templMedium)
{
    if(templLarge.empty() || templMedium.empty())
    {
        return;
    }
    templLarge_=templLarge(cv::Rect(222, 84, 127, 131)).clone();
    templMedium_=templMedium(cv::Rect(406, 47, 94, 89)).clone();
}

void TemplateCoinMatcher::computeRatio(const cv::Mat& coin4, const cv::Mat& coin6)
{
    cv::Mat dummyMask4 = cv::Mat::ones(coin4.size(), CV_8UC1) * 255;
    std::vector<cv::Rect> rects4 = getCoinRects(coin4, dummyMask4);
    if(rects4.size() == 4) {
        expectedRatio4_ = calculateAspectRatios(rects4);
        std::cout << ">>> Dynamic ratio calculated for 4 of Coins: " << expectedRatio4_ << std::endl;
    }

    cv::Mat dummyMask6 = cv::Mat::ones(coin6.size(), CV_8UC1) * 255;
    std::vector<cv::Rect> rects6 = getCoinRects(coin6, dummyMask6);
    if(rects6.size() == 6) {
        // Sort rects from top to bottom (lower y = higher)
        std::sort(rects6.begin(), rects6.end(), [](const cv::Rect& a, const cv::Rect& b){
            return a.y < b.y;
        });
        // Simulate the cut: take only the first 4
        std::vector<cv::Rect> top4Rects(rects6.begin(), rects6.begin() + 4);
        expectedRatio6_ = calculateAspectRatios(top4Rects);
        std::cout << ">>> Dynamic ratio calculated for 6 of Coins (cut): " << expectedRatio6_ << std::endl;
    }
}

double TemplateCoinMatcher::calculateAspectRatios(const std::vector<cv::Rect>& rects) {
    if (rects.size() != 4) return 0.0;
    
    double maxDist = 0;
    int idx1 = 0, idx2 = 0;
    
    std::vector<cv::Point> centers;
    for(auto& r : rects) {
        centers.push_back(cv::Point(r.x + r.width/2, r.y + r.height/2));
    }
    
    for(int i = 0; i < 4; i++) {
        for(int j = i+1; j < 4; j++) {
            double dist = cv::norm(centers[i] - centers[j]);
            if(dist > maxDist) {
                maxDist = dist;
                idx1 = i;
                idx2 = j;
            }
        }
    }
    
    std::vector<int> others;
    for(int i = 0; i < 4; i++) {
        if(i != idx1 && i != idx2) others.push_back(i);
    }
    
    if(others.size() != 2) return 0.0;
    
    double edge1 = cv::norm(centers[idx1] - centers[others[0]]);
    double edge2 = cv::norm(centers[idx1] - centers[others[1]]);
    
    double edgeLong = std::max(edge1, edge2);
    double edgeShort = std::min(edge1, edge2);
    
    if (edgeShort == 0) return 0.0;
    return edgeLong / edgeShort;
}

bool TemplateCoinMatcher::match(const cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card)
{
    std::vector<cv::Rect> coinRects;
    
    coinRects = getCoinRects(img, mask);
    int coin = coinRects.size();
    
    if (coin == 4 && expectedRatio4_ > 0 && expectedRatio6_ > 0) {
        double r = calculateAspectRatios(coinRects);
        if (std::abs(r - expectedRatio6_) < std::abs(r - expectedRatio4_)) {
            coin = 6;
            std::cout<<"CARD FOUND VIA TEMPLATE FALLBACK: 6 OF COINS (reconstructed from ratio "<< r <<")"<<std::endl;
        }
    }
    
    if (coin >= 2 && coin <= 7) {
        card = {Suit::COINS, coin};
        if (coin != 6 || coinRects.size() != 4) {
            std::cout<<"CARD FOUND VIA TEMPLATE FALLBACK: " << coin << " OF COINS"<<std::endl;
        }
        return true;
    }
    return false;
}

std::vector<cv::Rect> TemplateCoinMatcher::getCoinRects(const cv::Mat& img, const cv::Mat& mask) {
    if (templLarge_.empty() || templMedium_.empty()) return {};

    // Find the bounding box of the mask to optimize the search
    cv::Rect roi = cv::boundingRect(mask);
    if (roi.area() == 0) return {};
    
    // Expand the ROI slightly for safety
    roi.x = std::max(0, roi.x - 20);
    roi.y = std::max(0, roi.y - 20);
    roi.width = std::min(img.cols - roi.x, roi.width + 40);
    roi.height = std::min(img.rows - roi.y, roi.height + 40);

    cv::Mat imgROI = img(roi);
    cv::Mat maskROI = mask(roi);

    // Erode the mask to eliminate thin, fragmented shadows
    // on the static Briscola. The actual moving card is a solid blob.
    cv::Mat solidMask;
    cv::erode(maskROI, solidMask, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(21, 21)));

    std::vector<cv::Mat> templates = {templLarge_, templMedium_};
    
    struct Detection { cv::Rect rect; double score; };
    std::vector<Detection> all_detections;

    double scaleStart = 0.3;
    double scaleEnd = 1.5; 
    double scaleStep = 0.1;
    double thresholdScore = 0.60;

    for (const auto& t : templates) {
        for (double scale = scaleStart; scale <= scaleEnd; scale += scaleStep) {
            cv::Mat resizedTempl;
            cv::resize(t, resizedTempl, cv::Size(), scale, scale);

            if (resizedTempl.cols < 38 || resizedTempl.rows < 38 || (resizedTempl.cols * resizedTempl.rows) < 1500) {
                continue;
            }

            if (resizedTempl.cols > imgROI.cols || resizedTempl.rows > imgROI.rows) continue;

            cv::Mat result;
            cv::matchTemplate(imgROI, resizedTempl, result, cv::TM_CCOEFF_NORMED);
            cv::Mat resultThresh;
            cv::threshold(result, resultThresh, thresholdScore, 1.0, cv::THRESH_TOZERO);

            while (true) {
                double maxVal; cv::Point maxLoc;
                cv::minMaxLoc(resultThresh, nullptr, &maxVal, nullptr, &maxLoc);
                if (maxVal < thresholdScore) break;

                // Check: does the center of the template fall in the *solid* area of the motion mask?
                cv::Point center(maxLoc.x + resizedTempl.cols/2, maxLoc.y + resizedTempl.rows/2);
                if (solidMask.at<uchar>(center.y, center.x) > 128) {
                    all_detections.push_back({cv::Rect(roi.x + maxLoc.x, roi.y + maxLoc.y, resizedTempl.cols, resizedTempl.rows), maxVal});
                }

                int maskRadiusX = std::max(1, resizedTempl.cols / 2);
                int maskRadiusY = std::max(1, resizedTempl.rows / 2);
                cv::Point pt1(std::max(0, maxLoc.x - maskRadiusX), std::max(0, maxLoc.y - maskRadiusY));
                cv::Point pt2(std::min(resultThresh.cols - 1, maxLoc.x + maskRadiusX), std::min(resultThresh.rows - 1, maxLoc.y + maskRadiusY));
                cv::rectangle(resultThresh, pt1, pt2, cv::Scalar(0), cv::FILLED);
            }
        }
    }
    // NMS
    std::sort(all_detections.begin(), all_detections.end(), [](const Detection& a, const Detection& b) {
        return a.score > b.score;
    });

    std::vector<cv::Rect> final_rects;
    for (const auto& det : all_detections) {
        bool keep = true;
        for (const auto& final_rect : final_rects) {
            int interArea = (det.rect & final_rect).area();
            int unionArea = det.rect.area() + final_rect.area() - interArea;
            double iou = unionArea == 0 ? 0 : (double)interArea / unionArea;
            if (iou > 0.2) { 
                keep = false;
                break;
            }
        }
        if (keep) final_rects.push_back(det.rect);
    }

    return final_rects;
}
