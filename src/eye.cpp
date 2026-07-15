#include <eye.h>

Eye::Eye(int channelNum) : channelNum_(channelNum)
{
    recognizedCards_ = std::vector<std::pair<Suit, int>>();
    sift_ = cv::SIFT::create();
    plN_=false;
    matcher_ = cv::FlannBasedMatcher();
    
    //auto indexParams = cv::makePtr<cv::flann::LshIndexParams>(12,20,2);
    //auto searchParams = cv::makePtr<cv::flann::SearchParams>(50);
    //matcher_ = cv::FlannBasedMatcher(indexParams,searchParams);
}

void Eye::clear()
{
    recognizedCards_.clear();
    lastMask_ = cv::Mat();
    residualImage_ = cv::Mat();
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
    cv::Mat descriptors, allDescriptors, img;

    for (const std::tuple<cv::Mat, Suit, int>& card : trainingset){
        if(!isValidImage(std::get<0>(card)))
            throw std::invalid_argument("EyeError: At least an image in the dataset has not the right file format. "+std::to_string(channelNum_)+" channels images needed.");
    }

    for (const std::tuple<cv::Mat, Suit, int>& card : trainingset)
    {
        std::pair<Suit, int> p={std::get<1>(card), std::get<2>(card)};
        preprocessImage(std::get<0>(card), img);
        cv::split(img,channels);

        for(int i=0;i<channelNum_;i++)
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

    if (!isValidModelState())
    {
        reset();
        throw std::invalid_argument("EyeError: Dataset not usable for a Briscola match.");
    }
}

bool Eye::recognize(const cv::Mat& image, std::pair<Suit, int>& card)
{
    if (!isValidModelState())
        throw std::logic_error("EyeError: Attempt to call the model before training it.");
    if (!isValidImage(image))
        throw std::invalid_argument("EyeError: The image has not the right file format. "+std::to_string(channelNum_)+" channels images needed.");
    
    cv::Mat diffMask;
    bool found = recognizeCard(image, card, diffMask);

    if (!diffMask.empty()) {
        int hh = diffMask.rows / 2;
        cv::Mat up = diffMask(cv::Rect(0, 0, diffMask.cols, hh));
        cv::Mat bot = diffMask(cv::Rect(0, hh, diffMask.cols, diffMask.rows - hh));
        accumulatedUp_ += cv::countNonZero(up);
        accumulatedBot_ += cv::countNonZero(bot);
    }

    if(found)
    {
        if (accumulatedUp_ == 0 && accumulatedBot_ == 0 && !diffMask.empty()) {
            int hh = diffMask.rows / 2;
            cv::Mat up = diffMask(cv::Rect(0, 0, diffMask.cols, hh));
            cv::Mat bot = diffMask(cv::Rect(0, hh, diffMask.cols, diffMask.rows - hh));
            plN_ = (cv::countNonZero(up) > cv::countNonZero(bot));
        } else {
            plN_ = (accumulatedUp_ > accumulatedBot_);
        }
        
        accumulatedUp_ = 0;
        accumulatedBot_ = 0;
        
        card_=card;
        return true;
    }
    card=card_;
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
    if (img.channels()!=channelNum_)
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

bool Eye::findCardPosition(const cv::Mat& img, cv::Mat& mask_out){
    cv::Scalar mean, stddev;
    cv::Mat hsv, z_score, mask_high_sat, mask_lights;
    std::vector<cv::Mat> channels;
    std::vector<std::vector<cv::Point>> contours;

    cv::Mat kernel_3    = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3));
    cv::Mat kernel_7    = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7,7));
    cv::Mat kernel_11   = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(11,11));

    cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);
    cv::split(hsv, channels);

    cv::meanStdDev(channels[1],mean,stddev);
    z_score = (channels[1]-mean)/stddev;
    cv::threshold(z_score,mask_high_sat,0.85,255,cv::THRESH_BINARY);
    cv::morphologyEx(mask_high_sat, mask_high_sat, cv::MORPH_OPEN, kernel_3);
    cv::morphologyEx(mask_high_sat, mask_high_sat, cv::MORPH_CLOSE, kernel_11);

    cv::meanStdDev(channels[2],mean,stddev);
    cv::erode(channels[2], channels[2], kernel_3);
    z_score = (channels[2]-mean)/stddev;
    cv::threshold(z_score,mask_lights,0.97,255,cv::THRESH_BINARY);

    mask_out = mask_lights+mask_high_sat;
    
    cv::findContours(mask_out,contours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_SIMPLE);
    mask_out = cv::Mat::zeros(mask_high_sat.size(), CV_8UC1);
    for(int i=0; i<contours.size();i++)
    {
        if(cv::contourArea(contours[i])>800)
        cv::drawContours(mask_out,contours, i, cv::Scalar(255), cv::FILLED);
    }
    cv::morphologyEx(mask_out, mask_out, cv::MORPH_CLOSE, kernel_7);
    cv::erode(mask_out,mask_out,kernel_11);
    cv::dilate(mask_out,mask_out,kernel_3);
    
    if(cv::countNonZero(mask_out) < (img.rows*img.cols)*0.02)
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
    if(*maxCounts < 10) {
        if (!templatesLoaded_) loadTemplates();
        std::vector<cv::Rect> denariRects = getDenariRects(img, mask);
        int denari = denariRects.size();
        
        if (denari == 4 && expectedRatio4_ > 0 && expectedRatio6_ > 0) {
            double r = calculateAspectRatios(denariRects);
            if (std::abs(r - expectedRatio6_) < std::abs(r - expectedRatio4_)) {
                denari = 6;
                std::cout<<"CARTA TROVATA TRAMITE FALLBACK TEMPLATE: 6 DI DENARI (ricostruita dal ratio "<< r <<")"<<std::endl;
            }
        }
        
        if (denari >= 2 && denari <= 7) {
            card = {COINS, denari};
            if (denari != 6 || denariRects.size() != 4) {
                std::cout<<"CARTA TROVATA TRAMITE FALLBACK TEMPLATE: " << denari << " DI DENARI"<<std::endl;
            }
            return true;
        }
        return false;
    }

    card = cardVector_[imgIdx];
    std::cout<<"CARTA TROVATA (SIFT)"<<std::endl;
    return true;
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

    if(!findCardValue(img, diffMask, card))
        return false;

    residualImage_=img.clone();
    lastMask_ = mask.clone();
    return true;
}

void Eye::processMask(const cv::Mat& img, const cv::Mat& mask, cv::Mat& dst)
{
    dst = mask.clone();
    if (lastMask_.empty() || residualImage_.empty()) {
        return;
    }

    cv::Mat hsv_img, res_img_hsv;
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3));
    cv::Mat kernel_2 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5,5));
    cv::cvtColor(img,hsv_img,cv::COLOR_BGR2HSV);
    cv::cvtColor(residualImage_,res_img_hsv,cv::COLOR_BGR2HSV);
    cv::GaussianBlur(res_img_hsv,res_img_hsv,cv::Size(5,5),0);
    
    cv::Vec3b pix, img_pix, res_img_pix;
    uchar mask_pix;
    for(int i=0; i<mask.rows; i++)
    for(int j=0; j<mask.cols; j++)
    {
        if(mask.at<uchar>(i,j)==0)        
        {
            continue;
        }

        img_pix=hsv_img.at<cv::Vec3b>(i,j);
        res_img_pix=res_img_hsv.at<cv::Vec3b>(i,j);
        mask_pix=mask.at<uchar>(i,j)-lastMask_.at<uchar>(i,j);

        pix[0]=abs(img_pix[0]-res_img_pix[0]);
        pix[1]=abs(img_pix[1]-res_img_pix[1]);
        pix[2]=abs(img_pix[2]-res_img_pix[2]);

        if(mask_pix==0 && (pix[0]<35 || pix[1]<50))
        {
            dst.at<uchar>(i,j)=0;
        }
    }

    if(cv::countNonZero(dst) < (img.rows*img.cols)*0.012)
        dst=cv::Mat::zeros(dst.size(),dst.type());

    cv::erode(dst,dst,kernel);
    cv::dilate(dst,dst,kernel_2);
    cv::morphologyEx(dst, dst, cv::MORPH_CLOSE, kernel_2);

    cv::Mat render;
    img.copyTo(render,dst);
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

    // Calcolo dinamico ratio 4 vs 6
    cv::Mat img4 = cv::imread("../dataset/Briscola_Trentine/4-coins.JPG");
    cv::Mat img6 = cv::imread("../dataset/Briscola_Trentine/6-coins.JPG");
    if(!img4.empty() && !img6.empty()) {
        cv::Mat dummyMask4 = cv::Mat::ones(img4.size(), CV_8UC1) * 255;
        std::vector<cv::Rect> rects4 = getDenariRects(img4, dummyMask4);
        if(rects4.size() == 4) {
            expectedRatio4_ = calculateAspectRatios(rects4);
            std::cout << ">>> Ratio dinamico calcolato per il 4 di Denari: " << expectedRatio4_ << std::endl;
        }

        cv::Mat dummyMask6 = cv::Mat::ones(img6.size(), CV_8UC1) * 255;
        std::vector<cv::Rect> rects6 = getDenariRects(img6, dummyMask6);
        if(rects6.size() == 6) {
            // Ordina i rects dal più in alto al più in basso (y minore = più in alto)
            std::sort(rects6.begin(), rects6.end(), [](const cv::Rect& a, const cv::Rect& b){
                return a.y < b.y;
            });
            // Simuliamo il taglio: prendiamo solo i primi 4
            std::vector<cv::Rect> top4Rects(rects6.begin(), rects6.begin() + 4);
            expectedRatio6_ = calculateAspectRatios(top4Rects);
            std::cout << ">>> Ratio dinamico calcolato per il 6 di Denari (tagliato): " << expectedRatio6_ << std::endl;
        }
    }
}

std::vector<cv::Rect> Eye::getDenariRects(const cv::Mat& img, const cv::Mat& mask) {
    if (templLarge_.empty() || templMedium_.empty()) return {};

    // Troviamo il bounding box della maschera per ottimizzare la ricerca
    cv::Rect roi = cv::boundingRect(mask);
    if (roi.area() == 0) return {};
    
    // Per sicurezza, allarghiamo leggermente la ROI
    roi.x = std::max(0, roi.x - 20);
    roi.y = std::max(0, roi.y - 20);
    roi.width = std::min(img.cols - roi.x, roi.width + 40);
    roi.height = std::min(img.rows - roi.y, roi.height + 40);

    cv::Mat imgROI = img(roi);
    cv::Mat maskROI = mask(roi);

    // Erodiamo la maschera per eliminare le ombre sottili e frammentate
    // sulla Briscola statica. La vera carta in movimento è un blob solido.
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

                // Controllo: il centro del template cade nella zona *solida* della maschera di movimento?
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

double Eye::calculateAspectRatios(const std::vector<cv::Rect>& rects) {
    if (rects.size() != 4) return 0.0;
    
    std::vector<cv::Point> centers;
    for(const auto& r : rects) {
        centers.push_back(cv::Point(r.x + r.width/2, r.y + r.height/2));
    }
    
    double maxDist = 0;
    int idx1 = 0, idx2 = 0;
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
