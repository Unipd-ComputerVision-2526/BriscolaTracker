#include <eye.h>
#include <iostream>

Eye::Eye()
{
    cardMap_ = std::map<std::pair<Suit, int>, std::vector<cv::Mat>>();
    recognizedCards_ = std::vector<std::pair<Suit, int>>();
    sift_ = cv::SIFT::create();
    
    auto indexParams = cv::makePtr<cv::flann::LshIndexParams>(12,20,2);
    auto searchParams = cv::makePtr<cv::flann::SearchParams>(50);
    matcher_ = cv::FlannBasedMatcher();
    //matcher_ = cv::FlannBasedMatcher(indexParams,searchParams);
}

void Eye::clear()
{
    recognizedCards_.clear();
    recognizedBriscola = false;
    lastMask_ = cv::Mat();
}

void Eye::reset()
{
    clear();
    cardVector_.clear();
    cardMap_.clear();
}

void Eye::fit(const std::vector<std::tuple<cv::Mat, Suit, int>>& trainingset)
{
    std::vector<cv::Mat> channels;
    std::vector<cv::Mat> trainReadyDesc;
    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    cv::Mat allDescriptors;

    for (const std::tuple<cv::Mat, Suit, int>& card : trainingset){
        const cv::Mat& img = std::get<0>(card);
        if(!isValidImage(img))
            throw std::invalid_argument("EyeError: At least an image in the dataset has not the right file format. 3 channels images needed.");
    }

    for (const std::tuple<cv::Mat, Suit, int>& card : trainingset)
    {
        std::pair<Suit, int> p={std::get<1>(card), std::get<2>(card)};
        const cv::Mat& original_img = std::get<0>(card);
        cv::Mat img = preprocessImage(original_img);

        cv::split(img,channels);
        for(int i=0;i<3;i++)
        {
            sift_->detectAndCompute(channels[i], cv::noArray(), keypoints, descriptors);
            if(!descriptors.empty())
                allDescriptors.push_back(descriptors);
            descriptors.release();
        }

        allDescriptors.convertTo(allDescriptors,CV_32F);
        trainReadyDesc = {allDescriptors};
        matcher_.add(trainReadyDesc);
        cardVector_.push_back(p);

        keypoints.clear();
        allDescriptors.release();
    }
    matcher_.train();

    if (!validModelState())
    {
        cardVector_.clear();
        cardMap_.clear();
        throw std::invalid_argument("EyeError: dataset not usable for a Briscola match.");
    }
}

bool Eye::recognize(const cv::Mat& image, std::pair<Suit, int>& card)
{
    bool result=true;
    if (cardVector_.size()==0)
        throw std::logic_error("EyeError: Attempt to call the model before training it.");
    if (image.channels()!=3)
        throw std::invalid_argument("EyeError: Only 3 channels frame used for feature detection.");
    
    if (recognizedCards_.size()==0 || !recognizedBriscola)
        result = recognizeBriscola(image,card);
    else
        result = recognizeRoundCard(image,card);

    if(!result)
        return false;
    if(std::count(recognizedCards_.begin(),recognizedCards_.end(),card)!=0)
        return false;

    recognizedCards_.push_back(card);
    std::cout<<"TROVATO "<<recognizedCards_.size()<<std::endl;
    return true;
}

bool Eye::isValidImage(const cv::Mat& img){
    if (img.empty())
        return false;
    if (img.dims!=2)
        return false;
    if (img.depth() != CV_8U && img.depth() != CV_16U && img.depth() != CV_32F)
        return false;
    if (img.channels()!=3)
        return false;
    return true;
}

bool Eye::validModelState(){
    if (cardVector_.size()<40)
        return false;

    for (int suit=1; suit<5; suit++)
    {
        std::pair<Suit, int> p={static_cast<Suit>(suit), 1};
        for (; p.second<11; p.second++)
        {
            if (count(cardVector_.begin(),cardVector_.end(),p)!=1)
            {
                return false;
            }
        }
    }
    return true;
}

cv::Mat Eye::preprocessImage(const cv::Mat& img) {
    if (img.empty()) return img;

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

    return preprocessed_img;
}

bool Eye::findCardPosition(const cv::Mat& img, cv::Mat& mask)
{
    double min, max, filter;
    cv::Mat blurred;
    cv::Mat erosion_kernel = cv::getStructuringElement(
        cv::MORPH_RECT,
        cv::Size(5,5)
    );
    cv::Mat erosion_kernel_2 = cv::getStructuringElement(
        cv::MORPH_RECT,
        cv::Size(21,21)
    );
    cv::Mat dilate_kernel = cv::getStructuringElement(
        cv::MORPH_RECT,
        cv::Size(31,31)
    );
    cv::cvtColor(img,blurred,cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(blurred,blurred,cv::Size(23,23),0);

    cv::minMaxLoc(blurred, &min, &max);
    filter = min+(max-min)/1.5;

    cv::threshold(blurred,blurred,filter,255,cv::THRESH_BINARY);
    cv::erode(blurred, blurred, erosion_kernel);
    cv::dilate(blurred, blurred, dilate_kernel);
    cv::erode(blurred, mask, erosion_kernel_2);

    if(cv::countNonZero(mask) < (img.rows*img.cols)*0.02)
        return false;

    return true;
}

bool Eye::findCardValue(const cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card)
{
    std::vector<std::vector<cv::DMatch>> matches;
    std::vector<cv::KeyPoint> keypoints;
    std::vector<int>::iterator maxCounts;
    std::vector<int> matchCount(cardVector_.size());
    cv::Mat descriptors;
    float ratio = 0.75f;    //lowe's ratio
    int imgIdx, maxIdx;
    
    sift_->detectAndCompute(img,mask,keypoints,descriptors);

    cv::Mat img_keypoints;
    cv::drawKeypoints(img, keypoints, img_keypoints, cv::Scalar::all(-1), cv::DrawMatchesFlags::DEFAULT);
    cv::imshow("SIFT Descriptors", img_keypoints);

    matcher_.knnMatch(descriptors, matches, 2);

    for(int i=0; i<matches.size();i++)
    {
        if(matches[i].size() >= 2 && matches[i][0].distance < ratio*matches[i][1].distance)
        {
            imgIdx=matches[i][0].imgIdx;
            matchCount[imgIdx]++;
        }
    }

    for(int i=0; i<matchCount.size();i++) {
        if (i > 0 && i % 10 == 0) std::cout << "| ";
        std::cout<<matchCount[i]<<" ";
    }
    std::cout<<std::endl;

    maxCounts = std::max_element(matchCount.begin(),matchCount.end());
    imgIdx = std::distance(matchCount.begin(),maxCounts);
    card = cardVector_[imgIdx];

    if(*maxCounts < 10)
        return false;
    std::cout<<"CARTA TROVATA"<<std::endl;
    return true;
}

bool Eye::recognizeBriscola(const cv::Mat& img, std::pair<Suit, int>& card)
{
    cv::Mat rescaled;

    cv::resize(img, rescaled, cv::Size(img.cols/3, img.rows/3));
    if(!findCardPosition(rescaled, lastMask_))
        return false;
    cv::resize(lastMask_, lastMask_, cv::Size(lastMask_.cols*3, lastMask_.rows*3));
    if (!findCardValue(img, lastMask_, card))
        return false;

    recognizedBriscola=true;
    return true;
}

bool Eye::recognizeRoundCard(const cv::Mat& img, std::pair<Suit, int>& card)
{
    cv::Mat rescaled, mask, diffMask;

    cv::resize(img, rescaled, cv::Size(img.cols/3, img.rows/3));
    findCardPosition(rescaled, mask);
    cv::resize(mask, mask, cv::Size(mask.cols*3, mask.rows*3));
    
    if (lastMask_.empty()) {
        diffMask = mask.clone();
    } else {
        diffMask = mask - lastMask_;
    }
    
    cv::imshow("Difference Mask", diffMask);

    if(!findCardValue(img, diffMask, card))
        return false;

    lastMask_ = mask.clone();
    return true;
}
