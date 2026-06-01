#include <eye.h>
#include <iostream>

Eye::Eye()
{
    cardMap_ = std::map<std::pair<Suit, int>, std::vector<cv::Mat>>();
    recognizedCards_ = std::vector<std::pair<Suit, int>>();
    fast_  = cv::FastFeatureDetector::create();
    sift_ = cv::SIFT::create();
    matcher_ = cv::FlannBasedMatcher();
}

void Eye::clear()
{
    cardMap_.clear();
    recognizedCards_.clear();
}

void Eye::fit(const std::vector<std::tuple<cv::Mat, Suit, int>>& trainingset)
{
    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;

    for (const std::tuple<cv::Mat, Suit, int>& card : trainingset){
        const cv::Mat& img = std::get<0>(card);
        if(!isValidImage(img))
            throw std::invalid_argument("EyeError: At least an image in the dataset has not the right file format. Grayscale images needed.");
    }

    for (const std::tuple<cv::Mat, Suit, int>& card : trainingset)
    {
        std::pair<Suit, int> p={std::get<1>(card), std::get<2>(card)};
        std::cout<<p.first<<" "<<p.second<<std::endl;
        const cv::Mat& img = std::get<0>(card);

        //fast_->detect(img, keypoints);
        //sift_->compute(img, keypoints, descriptors);
        
        sift_->detectAndCompute(img, cv::noArray(), keypoints, descriptors);

        matcher_.add(descriptors);
        cardVector_.push_back(p);
        //cardMap_.insert({p, descriptors});
    }
    matcher_.train();

    if (!validModelState())
    {
        cardVector_.clear();
        //cardMap_.clear();
        throw std::invalid_argument("EyeError: dataset not usable for a Briscola match.");
    }
}

bool Eye::recognize(const cv::Mat& image, std::pair<Suit, int>& card)
{
    bool result;
    if (cardVector_.size()==0)
    //if (cardMap_.size()==0)
        throw std::logic_error("EyeError: Attempt to call the model before training it.");
    if (image.channels()!=1)
        throw std::invalid_argument("EyeError: Only grayscale frame used for feature detection.");
    
    if (recognizedCards_.size()==0)
        result = recognizeBriscola(image,card);
    else
        result = recognizeRoundCard(image,card);

    if(std::count(recognizedCards_.begin(),recognizedCards_.end(),card)!=0)
        return false;

    recognizedCards_.push_back(card);
    return result;
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
    if (cardVector_.size()<40)
    //if (cardMap_.size()<40)
        return false;

    for (int suit=1; suit<5; suit++)
    {
        for (int v=1; v<11; v++)
        {
            std::pair<Suit, int> p={static_cast<Suit>(suit), v};
            if (count(cardVector_.begin(),cardVector_.end(),p)!=1)
            //if (cardMap_.count(p)!=1)
            {
                return false;
            }
        }
    }
    return true;
}

bool Eye::findCardPosition(const cv::Mat& img, cv::Mat& mask)
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
    //cv::cvtColor(img,mask,cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(img,blurred,cv::Size(23,23),0);

    cv::minMaxLoc(blurred, &min, &max);
    filter = min+(max-min)/1.5;

    cv::threshold(blurred,blurred,filter,255,cv::THRESH_BINARY);
    cv::erode(blurred, blurred, erosion_kernel);
    cv::dilate(blurred, mask, dilate_kernel);

    return true;
}

bool Eye::findCardValue(const cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card)
{
    std::vector<std::vector<cv::DMatch>> matches;
    std::vector<cv::KeyPoint> keypoints;
    std::vector<int>::iterator maxCounts;
    std::vector<int> matchCount(cardVector_.size());
    cv::Mat descriptors;
    float ratio = 0.75f;
    int imgIdx, maxIdx;

    //fast_->detect(img,keypoints,mask);
    //sift_->compute(img,keypoints,descriptors);

    sift_->detectAndCompute(img, mask, keypoints, descriptors);
    
    cv::Mat img_keypoints;
    cv::drawKeypoints(img, keypoints, img_keypoints, cv::Scalar::all(-1), cv::DrawMatchesFlags::DEFAULT);
    cv::imshow("FAST Keypoints with SIFT Descriptors", img_keypoints);

    matcher_.knnMatch(descriptors, matches, 2);
    for(int i=0; i<matches.size();i++)
    {
        if(matches[i].size() >= 2 && matches[i][0].distance < ratio*matches[i][1].distance)
        {
            imgIdx=matches[i][0].imgIdx;
            matchCount[imgIdx]++;
        }
    }

    maxCounts = std::max_element(matchCount.begin(),matchCount.end());
    imgIdx = std::distance(matchCount.begin(),maxCounts);
    card = cardVector_[imgIdx];

    if(*maxCounts == 0)
        return false;

    return true;
}

bool Eye::recognizeBriscola(const cv::Mat& img, std::pair<Suit, int>& card)
{
    cv::Mat rescaled;

    cv::resize(img, rescaled, cv::Size(img.cols/3, img.rows/3));
    findCardPosition(rescaled, lastMask_);
    if (!findCardValue(rescaled, lastMask_, card))
        return false;
    return true;
}

bool Eye::recognizeRoundCard(const cv::Mat& img, std::pair<Suit, int>& card)
{
    cv::Mat rescaled, mask, diffMask;

    cv::resize(img, rescaled, cv::Size(img.cols/3, img.rows/3));
    findCardPosition(rescaled, mask);
    diffMask = mask - lastMask_;
    
    cv::imshow("",diffMask);

    findCardValue(rescaled, diffMask, card);

    lastMask_ = mask.clone();
    return true;
}
