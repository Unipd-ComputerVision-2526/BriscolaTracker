#include <eye.h>

Eye::Eye()
{
    sift_ = cv::SIFT::create();
    akaze_ = cv::AKAZE::create();
    plN_=false;
    fl_matcher_ = cv::FlannBasedMatcher();
    bf_matcher_ = cv::BFMatcher(cv::NORM_HAMMING2);
}

void Eye::clear()
{
    residualImage_ =cv::Mat();
    lastMask_ = cv::Mat();
    accumulatedUp_ = 0;
    accumulatedBot_ = 0;
}

void Eye::setBaseline(const cv::Mat& img)
{
    if (img.empty()) return;
    cv::Mat rescaled, mask;
    cv::resize(img, rescaled, cv::Size(img.cols/3, img.rows/3));
    if(findCardPosition(rescaled, mask)) {
        cv::resize(mask, mask, cv::Size(mask.cols*3, mask.rows*3));
        lastMask_ = mask.clone();
        residualImage_ = img.clone();
    }
}

void Eye::reset()
{
    clear();
    cardVector_.clear();
}

void Eye::fit(const std::vector<std::tuple<cv::Mat, Suit, int>>& trainingset)
{
    std::vector<cv::KeyPoint> keypoints;
    std::vector<cv::Mat> channels, trainReadyDesc;
    cv::Mat img_cp, descriptors, si_all_descriptors, ak_all_descriptors, akaze_ready;
    cv::Mat templMedium_,templLarge_;

    for (const std::tuple<cv::Mat, Suit, int>& card : trainingset){
        if(!isValidImage(std::get<0>(card)))
            throw std::invalid_argument("EyeError: At least an image in the dataset has not the right file format. GBR channels images needed.");
    }

    for (const std::tuple<cv::Mat, Suit, int>& card : trainingset)
    {
        img_cp=std::get<0>(card).clone();
        std::pair<Suit, int> p={std::get<1>(card), std::get<2>(card)};
        preprocessImage(img_cp, akaze_ready);

        sift_->detectAndCompute(std::get<0>(card), cv::noArray(), keypoints, descriptors);
        if(!descriptors.empty())
            si_all_descriptors.push_back(descriptors);
        descriptors.release();

        cv::cvtColor(akaze_ready,akaze_ready,cv::COLOR_BGR2Lab);
        cv::split(akaze_ready,channels);

        for(int i=1;i<=2;i++)
        {
            akaze_->detectAndCompute(channels[i], cv::noArray(), keypoints, descriptors);
            if(!descriptors.empty())
                ak_all_descriptors.push_back(descriptors);
            descriptors.release();
        }

        fl_matcher_.add({si_all_descriptors});
        bf_matcher_.add({ak_all_descriptors});

        cardVector_.push_back(p);
        si_all_descriptors.release();
        ak_all_descriptors.release();

        if(p.first==Suit::COINS)
        {
            switch(p.second)
            {
                case 3:
                    templLarge_ = img_cp(cv::Rect(222, 84, 127, 131)).clone();
                    break;
                case 4:
                    coin4=img_cp.clone();
                    break;
                case 6:
                    coin6=img_cp.clone();
                    break;
                case 7:
                    templMedium_ = img_cp(cv::Rect(406, 47, 94, 89)).clone();
                    break;
            }
            if(!templLarge_.empty() && !templMedium_.empty())
                templatesLoaded_ = true;
        }
    }
    fl_matcher_.train();
    
    if (!isValidModelState())
    {
        reset();
        throw std::invalid_argument("EyeError: Dataset not usable for a Briscola match.");
    }

    tc_matcher_.addTemplates(templLarge_,templMedium_);
    tc_matcher_.computeRatio(coin4,coin6);
}

bool Eye::recognize(const cv::Mat& image, std::pair<Suit, int>& card)
{
    if (!isValidModelState())
        throw std::logic_error("EyeError: Attempt to call the model before training it.");
    if (!isValidImage(image))
        throw std::invalid_argument("EyeError: The image has not the right file format. GBR images needed.");
    
    cv::Mat diffMask;
    bool found = recognizeCard(image,card,diffMask);
    accumulateMotion(diffMask, found);

    if(found)
    {
        card_=card;
        return true;
    }
    getLastCard(card);
    return false;
}

bool Eye::getLastCard(std::pair<Suit, int>& card)
{
    if(!lastMask_.empty())
    {
        card=card_;
        return true;
    }
    return false;
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

bool Eye::isValidModelState(){
    if (cardVector_.size()!=40)
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

void Eye::preprocessImage(const cv::Mat& img, cv::Mat& dst) {
    if (img.empty())
    {
        dst=cv::Mat();
        return;
    }

    std::vector<cv::Mat> channels;
    cv::Mat lab_img, local_mean, chan_float, mask_high_sat, preprocessed_img;

    cv::cvtColor(img, lab_img, cv::COLOR_BGR2Lab);
    cv::split(lab_img, channels);
    
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(3.0);
    clahe->setTilesGridSize(cv::Size(8, 8));
    
    clahe->apply(channels[0], channels[0]);

    cv::boxFilter(channels[2],local_mean,CV_32F, cv::Size(11,11));
    local_mean = 128.0f-local_mean;
    channels[2].convertTo(chan_float,CV_32F);
    chan_float+=local_mean;
    chan_float.convertTo(channels[2],CV_8U);

    cv::merge(channels, preprocessed_img);
    cv::cvtColor(preprocessed_img, preprocessed_img, cv::COLOR_Lab2BGR);

    dst=preprocessed_img.clone();
}

bool Eye::findCardPosition(const cv::Mat& img, cv::Mat& mask_out){
    cv::Scalar mean, stddev;
    cv::Mat hsv_img, z_score, mask_high_sat, mask_lights;
    std::vector<cv::Mat> channels;
    std::vector<std::vector<cv::Point>> contours;

    cv::Mat kernel_3    = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3));
    cv::Mat kernel_7    = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7,7));
    cv::Mat kernel_11   = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(11,11));

    cv::cvtColor(img, hsv_img, cv::COLOR_BGR2HSV);
    cv::split(hsv_img, channels);

    cv::meanStdDev(channels[1],mean,stddev);
    z_score = (channels[1]-mean)/stddev;
    cv::threshold(z_score,mask_high_sat,0.98,255,cv::THRESH_BINARY);
    cv::morphologyEx(mask_high_sat, mask_high_sat, cv::MORPH_OPEN, kernel_3);

    cv::meanStdDev(channels[2],mean,stddev);
    cv::erode(channels[2], channels[2], kernel_3);
    z_score = (channels[2]-mean)/stddev;
    cv::threshold(z_score,mask_lights,0.97,255,cv::THRESH_BINARY);

    mask_lights = mask_lights+mask_high_sat;

    cv::findContours(mask_lights,contours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_SIMPLE);
    mask_out = cv::Mat::zeros(mask_lights.size(), CV_8UC1);
    for(int i=0; i<contours.size();i++)
    {
        if(cv::contourArea(contours[i])>800)
        cv::drawContours(mask_out,contours, i, cv::Scalar(255), cv::FILLED);
    }
    cv::morphologyEx(mask_out, mask_out, cv::MORPH_CLOSE, kernel_7);
    //cv::morphologyEx(mask_out, mask_out, cv::MORPH_CLOSE, kernel_11);
    //cv::erode(mask_out,mask_out,kernel_11);
    //cv::dilate(mask_out,mask_out,kernel_3);

    if(cv::countNonZero(mask_out) < (img.rows*img.cols)*0.01)
        return false;
    return true;
}

bool Eye::findCardValue(const cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card)
{
    bool result;
    cv::Mat sift_ready, akaze_ready;

    sift_ready=img.clone();
    preprocessImage(img,akaze_ready);

    result=siftRecognition(sift_ready, mask, card);
    if(!result){
        result=akazeRecognition(akaze_ready, mask, card);
    }
    if(!result){
        result = tc_matcher_.match(img, mask, card);
    }

    return result;
}

bool Eye::recognizeCard(const cv::Mat& img, std::pair<Suit, int>& card, cv::Mat& diffMask)
{
    cv::Mat rescaled, mask;

    cv::resize(img, rescaled, cv::Size(img.cols/3, img.rows/3));
    if(!findCardPosition(rescaled, mask))
    {
        return false;
    }

    cv::resize(mask, mask, cv::Size(mask.cols*3, mask.rows*3));
    processMask(img, mask, diffMask);

    cv::namedWindow("img",cv::WINDOW_NORMAL);
    cv::imshow("img",rescaled);
    cv::resizeWindow("img", cv::Size(400,700));
    cv::namedWindow("mask",cv::WINDOW_NORMAL);
    cv::imshow("mask",diffMask);
    cv::resizeWindow("mask", cv::Size(400,700));
    cv::waitKey(0);

    if(!findCardValue(img, diffMask, card))
        return false;

    residualImage_=img.clone();
    lastMask_ = mask.clone();
    return true;
}

void Eye::processMask(const cv::Mat& img, const cv::Mat& mask, cv::Mat& dst)
{
    if (lastMask_.empty() || residualImage_.empty()) {
        dst = mask.clone();
        return;
    }

    double thresh, k=2;
    cv::Scalar global_mean, global_stddev;
    cv::Mat hsv_img, res_img_hsv, diff_img, pixel_mean, diff_mask;
    cv::Mat kernel_3 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3));
    cv::Mat kernel_5 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5,5));

    cv::cvtColor(img,hsv_img,cv::COLOR_BGR2HSV);
    cv::cvtColor(residualImage_,res_img_hsv,cv::COLOR_BGR2HSV);

    cv::GaussianBlur(res_img_hsv,res_img_hsv,cv::Size(5,5),0);
    cv::absdiff(hsv_img,res_img_hsv,diff_img);
    
    cv::transform(diff_img, pixel_mean, cv::Matx13f(1.f/3.f, 1.f/3.f, 1.f/3.f));
    cv::meanStdDev(pixel_mean,global_mean,global_stddev);
    thresh= global_mean[0] + k*global_stddev[0];

    thresh= thresh>40 ? thresh : 255;
    cv::threshold(pixel_mean,diff_mask,thresh,255,cv::THRESH_BINARY);
    diff_mask=lastMask_&diff_mask;
    dst=(mask-lastMask_)+diff_mask;

    if(cv::countNonZero(dst) < (img.rows*img.cols)*0.012)
        dst=cv::Mat::zeros(dst.size(),dst.type());

    cv::erode(dst,dst,kernel_3);
    cv::dilate(dst,dst,kernel_5);
}

void Eye::accumulateMotion(const cv::Mat& diffMask, bool reset)
{
    int zone_sep = 2*diffMask.rows/5;
    cv::Mat up=diffMask(cv::Rect(0, 0, diffMask.cols, zone_sep));
    cv::Mat bot=diffMask(cv::Rect(0, diffMask.rows-zone_sep, diffMask.cols, zone_sep));
    
    accumulatedUp_ += cv::countNonZero(up);
    accumulatedBot_ += cv::countNonZero(bot);
    
    if(reset){
        plN_=accumulatedUp_>accumulatedBot_;
        accumulatedUp_ = 0;
        accumulatedBot_ = 0;
    }
    
}

bool Eye::siftRecognition(cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card)
{
    std::vector<std::vector<cv::DMatch>> matches;
    std::vector<cv::KeyPoint> keypoints;
    std::vector<int> matchCount(cardVector_.size());
    std::vector<int>::iterator maxCounts;
    cv::Mat descriptors, allDescriptors;
    float ratio = 0.75f;
    int imgIdx;

    sift_->detectAndCompute(img, mask, keypoints, descriptors);
    if(!descriptors.empty())
        allDescriptors.push_back(descriptors);
    else
        return false;

    fl_matcher_.knnMatch(allDescriptors, matches, 2);
    
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

    if(*maxCounts < 10)
        return false;

    card = cardVector_[imgIdx];
    std::cout<<"recognized by sift"<<std::endl;

    return true;
}

bool Eye::akazeRecognition(cv::Mat& img, const cv::Mat& mask, std::pair<Suit, int>& card)
{
    std::vector<std::vector<cv::DMatch>> matches;
    std::vector<cv::KeyPoint> keypoints;
    std::vector<cv::Mat> channels;
    std::vector<int> matchCount(cardVector_.size());
    std::vector<int>::iterator maxCounts;
    cv::Mat descriptors, allDescriptors;
    int imgIdx, distance = 60;

    cv::cvtColor(img,img,cv::COLOR_BGR2Lab);
    cv::split(img,channels);

    for(int i=1; i<=2; i++)
    {
        akaze_->detectAndCompute(channels[i], mask, keypoints, descriptors);
        if(!descriptors.empty())
            allDescriptors.push_back(descriptors);
    }

    if (allDescriptors.empty())
        return false;

    bf_matcher_.knnMatch(allDescriptors, matches, 2);
    
    for(int i=0; i<matches.size();i++)
    {
        if(matches[i].size() >= 2 && matches[i][0].distance < distance)
        {
            imgIdx=matches[i][0].imgIdx;
            matchCount[imgIdx]++;
        }
    }

    maxCounts = std::max_element(matchCount.begin(),matchCount.end());
    imgIdx = std::distance(matchCount.begin(),maxCounts);

    if(*maxCounts < 10)
        return false;

    card = cardVector_[imgIdx];

    if(card.first==Suit::COINS){
        int c=circleCounter(img, mask);
        if(c<8 && (card.second<c || (card.second>7 && c>=4))){
            card.second=c;
            return true;
        }
        std::vector<int>::iterator secMaxCounts=std::find(maxCounts + 1, matchCount.end(), *maxCounts);
        if(*maxCounts == *secMaxCounts){
            imgIdx = std::distance(matchCount.begin(),secMaxCounts);
            std::cout<<cardVector_[imgIdx].first<<" "<<cardVector_[imgIdx].second<<std::endl;
            if(cardVector_[imgIdx].first==card.first && cardVector_[imgIdx].second>card.second && cardVector_[imgIdx].second<8)
                card.second=cardVector_[imgIdx].second;
        }
    }

    std::cout<<"recognized by akaze"<<std::endl;

    return true;
}

int Eye::circleCounter(const cv::Mat& img, const cv::Mat& mask)
{
    int hough_circle_dp=1;
    int hough_circle_min_dist=30;
    int hough_slider_circle_1=100;
    int hough_slider_circle_2=30;
    int minRadius= 15;
    int maxRadius= 30;

    cv::Mat edges, gray, gray_cp;
    std::vector<cv::Vec3f> circles;
    cv::cvtColor(img,gray,cv::COLOR_Lab2BGR);
    cv::cvtColor(gray,gray,cv::COLOR_BGR2GRAY);

    gray.copyTo(gray_cp,mask);
    cv::Canny(gray_cp, edges, 50, 200);
    cv::HoughCircles(edges, circles, cv::HOUGH_GRADIENT, hough_circle_dp, hough_circle_min_dist, hough_slider_circle_1, hough_slider_circle_2, minRadius, maxRadius);

    for(size_t i=0;i<circles.size();i++)
    {
        cv::circle(gray_cp,cv::Point(cvRound(circles[i][0]),cvRound(circles[i][1])),cvRound(circles[i][2]),cv::Scalar(0,255,0),1);
    }

    return circles.size();
}
