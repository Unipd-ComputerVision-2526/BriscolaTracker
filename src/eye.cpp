#include <eye.h>
#include <iostream>

Eye::Eye()
{
    //cardMap_ = std::map<std::pair<Suit, int>, std::vector<cv::Mat>>();
    recognizedCards_ = std::vector<std::pair<Suit, int>>();
    sift_ = cv::SIFT::create();
    
    //auto indexParams = cv::makePtr<cv::flann::LshIndexParams>(12,20,2);
    //auto searchParams = cv::makePtr<cv::flann::SearchParams>(50);
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
    //cardMap_.clear();
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
        cv::Mat img;
        preprocessImage(original_img, img);

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
        reset();
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

    residualImage_=image.clone();

    if(!result)
        return false;
    if(std::count(recognizedCards_.begin(),recognizedCards_.end(),card)!=0)
        return false;

    recognizedCards_.push_back(card);
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

void Eye::preprocessImage(const cv::Mat& img, cv::Mat& dst) {
    if (img.empty())
    {
        dst=img.clone();
        return;
    }

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

    dst=preprocessed_img.clone();
}

bool Eye::findCardPosition(const cv::Mat& img, cv::Mat& mask)
{
    cv::Mat gray;
    cv::Mat er_kernel_1 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7,7));
    cv::Mat er_kernel_2 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5,5));
    cv::Mat dil_kernel_1 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5,31));
    cv::Mat dil_kernel_2 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(21,9));
    cv::Mat clo_kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(15,15));
    
    cv::medianBlur(img,gray, 5);
    cv::erode(gray, gray, er_kernel_1);

    cv::cvtColor(gray, gray, cv::COLOR_BGR2GRAY);
    double min, max;
    cv::minMaxLoc(gray, &min, &max);
    double filter = min+(max-min)/1.3;
    cv::threshold(gray,mask,filter,255,cv::THRESH_BINARY);
    
    cv::erode(mask, mask, er_kernel_2);
    cv::dilate(mask, mask, dil_kernel_2);
    cv::dilate(mask, mask, dil_kernel_1);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, clo_kernel);
    
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
    int imgIdx;
    
    sift_->detectAndCompute(img,mask,keypoints,descriptors);

    /*cv::Mat img_keypoints;
    cv::drawKeypoints(img, keypoints, img_keypoints, cv::Scalar::all(-1), cv::DrawMatchesFlags::DEFAULT);
    cv::imshow("SIFT Descriptors", img_keypoints);
    cv::waitKey(0);*/

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

    if(*maxCounts < 10) {
        if (!templatesLoaded_) loadTemplates();
        int denari = countDenari(img, mask);
        if (denari >= 2 && denari <= 7) {
            card = {COINS, denari};
            std::cout<<"CARTA TROVATA TRAMITE FALLBACK TEMPLATE: " << denari << " DI DENARI"<<std::endl;
            return true;
        }
        return false;
    }
    std::cout<<"CARTA TROVATA (SIFT)"<<std::endl;
    return true;
}

bool Eye::recognizeBriscola(const cv::Mat& img, std::pair<Suit, int>& card)
{
    cv::Mat rescaled;

    cv::resize(img, rescaled, cv::Size(img.cols/3, img.rows/3));
    if(!findCardPosition(rescaled, lastMask_))
    {
        return false;
    }

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
    
    processMask(img, mask, diffMask);
    lastMask_ = mask.clone();

    return findCardValue(img, diffMask, card);
}

void Eye::processMask(const cv::Mat& img, const cv::Mat& mask, cv::Mat& dst)
{
    dst = mask.clone();
    if (lastMask_.empty() || residualImage_.empty()) {
        return;
    }
    
    cv::Vec3b pix;
    uchar mask_pix;
    for(int i=0; i<mask.rows; i++)
    for(int j=0; j<mask.cols; j++)
    {
        if(mask.at<uchar>(i,j)==0)        
        {
            continue;
        }

        mask_pix=mask.at<uchar>(i,j)-lastMask_.at<uchar>(i,j);
        pix[0]=abs(img.at<cv::Vec3b>(i,j)[0]-residualImage_.at<cv::Vec3b>(i,j)[0]);
        pix[1]=abs(img.at<cv::Vec3b>(i,j)[1]-residualImage_.at<cv::Vec3b>(i,j)[1]);
        pix[2]=abs(img.at<cv::Vec3b>(i,j)[2]-residualImage_.at<cv::Vec3b>(i,j)[2]);

        if(mask_pix==0 && cv::norm(pix,cv::NORM_L2) < 10)
        {
            dst.at<uchar>(i,j)=0;
        }
    }

    if(cv::countNonZero(dst) < (img.rows*img.cols)*0.02)
        dst=cv::Mat::zeros(dst.size(),dst.type());

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7,7));
    cv::erode(dst,dst,kernel);
}

void Eye::loadTemplates() {
    std::string pathLarge = "../dataset/Briscola_Trentine/3-coins.JPG";
    cv::Mat sourceLarge = cv::imread(pathLarge);
    if (!sourceLarge.empty()) {
        templLarge_ = sourceLarge(cv::Rect(222, 84, 127, 131)).clone();
    } else {
        std::cerr << "EyeError: Impossibile caricare " << pathLarge << std::endl;
    }

    std::string pathMedium = "../dataset/Briscola_Trentine/7-coins.JPG";
    cv::Mat sourceMedium = cv::imread(pathMedium);
    if (!sourceMedium.empty()) {
        templMedium_ = sourceMedium(cv::Rect(406, 47, 94, 89)).clone();
    } else {
        std::cerr << "EyeError: Impossibile caricare " << pathMedium << std::endl;
    }
    templatesLoaded_ = true;
}

int Eye::countDenari(const cv::Mat& img, const cv::Mat& mask) {
    if (templLarge_.empty() || templMedium_.empty()) return 0;

    // Troviamo il bounding box della maschera per ottimizzare la ricerca
    cv::Rect roi = cv::boundingRect(mask);
    if (roi.area() == 0) return 0;
    
    // Per sicurezza, allarghiamo leggermente la ROI
    roi.x = std::max(0, roi.x - 20);
    roi.y = std::max(0, roi.y - 20);
    roi.width = std::min(img.cols - roi.x, roi.width + 40);
    roi.height = std::min(img.rows - roi.y, roi.height + 40);

    cv::Mat imgROI = img(roi);
    cv::Mat maskROI = mask(roi);

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

                // Controllo: il centro del template trovato cade dentro la mask fornita da Eye?
                // Se sì, è una moneta sulla carta, non sulla tovaglia o sfondo.
                cv::Point center(maxLoc.x + resizedTempl.cols/2, maxLoc.y + resizedTempl.rows/2);
                if (maskROI.at<uchar>(center.y, center.x) > 128) {
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

    std::vector<Detection> final_detections;
    for (const auto& det : all_detections) {
        bool keep = true;
        for (const auto& final_det : final_detections) {
            int interArea = (det.rect & final_det.rect).area();
            int unionArea = det.rect.area() + final_det.rect.area() - interArea;
            double iou = unionArea == 0 ? 0 : (double)interArea / unionArea;
            if (iou > 0.2) { 
                keep = false;
                break;
            }
        }
        if (keep) final_detections.push_back(det);
    }

    return final_detections.size();
}
