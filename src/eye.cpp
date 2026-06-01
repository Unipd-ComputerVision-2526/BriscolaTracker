#include <eye.h>
#include <iostream>

Eye::Eye()
{
    cardMap_ = std::map<std::pair<Suit, int>, std::vector<cv::KeyPoint>>();
    recognizedCards_ = std::vector<std::pair<Suit, int>>();
    fast_ = cv::FastFeatureDetector::create();
}

void Eye::clear()
{
    cardMap_.clear();
    recognizedCards_.clear();
}

void Eye::fit(const std::vector<std::tuple<cv::Mat, Suit, int>>& trainingset)
{
    std::vector<cv::KeyPoint> keypoints;

    for (const std::tuple<cv::Mat, Suit, int>& card : trainingset){
        const cv::Mat& img = std::get<0>(card);
        if(!isValidImage(img))
            throw std::invalid_argument("EyeError: At least an image in the dataset has not the right file format. Grayscale images needed.");
    }

    for (const std::tuple<cv::Mat, Suit, int>& card : trainingset)
    {
        std::pair<Suit, int> p={std::get<1>(card), std::get<2>(card)};
        const cv::Mat& img = std::get<0>(card);

        fast_->detect(img, keypoints);
        cardMap_.insert({p, keypoints});
    }

    if (!validModelState())
    {
        cardMap_.clear();
        throw std::invalid_argument("EyeError: dataset not usable for a Briscola match.");
    }
}

void Eye::recognize(const cv::Mat& image, std::pair<Suit, int>& card)
{
    if (cardMap_.size()==0)
        throw std::logic_error("EyeError: Attempt to call the model before training it.");
    if (image.channels()!=1)
        throw std::invalid_argument("EyeError: Only grayscale frame used for feature detection.");
    
    if (recognizedCards_.size()==0)
        return recognizeBriscola(image,card);
    else
        return recognizeRoundCard(image,card);
}

bool Eye::isValidImage(const cv::Mat& img){
    if (img.empty())
        return false;
    if (img.dims!=2)
        return false;
    if (img.depth() != CV_8U && img.depth() != CV_16U && img.depth() != CV_32F)
        return false;
    if (img.channels()!=1)
        return false;
    return true;
}

bool Eye::validModelState(){
    if (cardMap_.size()<40)
        return false;

    for (int suit=1; suit<5; suit++)
    {
        for (int v=1; v<11; v++)
        {
            std::pair<Suit, int> p={static_cast<Suit>(suit), v};
            if (cardMap_.count(p)!=1)
            {
                return false;
            }
        }
    }
    return true;
}

void Eye::findCardPosition(const cv::Mat& img, cv::Mat& mask)
{
    double min, max, filter;
    cv::Mat blurred;
    cv::Mat erosion_kernel = cv::getStructuringElement(
        cv::MORPH_RECT,
        cv::Size(5,5)
    );
    cv::Mat dilate_kernel = cv::getStructuringElement(
        cv::MORPH_RECT,
        cv::Size(31,31)
    );
    cv::cvtColor(img,mask,cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(mask,blurred,cv::Size(23,23),0);

    cv::minMaxLoc(blurred, &min, &max);
    filter = min+(max-min)/1.5;

    cv::threshold(blurred,blurred,filter,256,cv::THRESH_BINARY);
    cv::erode(blurred, blurred, erosion_kernel);
    cv::dilate(blurred, mask, dilate_kernel);
}

void findCardValue(const cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card)
{
    for(int suit=1; suit<5; suit++)
    {
        //TODO
    }
}

void Eye::recognizeBriscola(const cv::Mat& img, std::pair<Suit, int>& card)
{
    cv::Mat rescaled;
    cv::resize(img, rescaled, cv::Size(img.cols/3, img.rows/3));
    findCardPosition(rescaled, lastMask_);
    findCardValue(rescaled, lastMask_, card);
    
    //cv::imshow("",mask);
}

void Eye::recognizeRoundCard(const cv::Mat& img, std::pair<Suit, int>& card)
{
    //TODO
}




