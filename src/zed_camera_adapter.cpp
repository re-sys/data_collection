#include "zed_camera_adapter.hpp"

#ifdef ZED_AVAILABLE

#include <opencv2/imgproc.hpp>

ZEDCameraAdapter::ZEDCameraAdapter() {
}

ZEDCameraAdapter::~ZEDCameraAdapter() {
    closeCamera();
}

bool ZEDCameraAdapter::openCamera() {
    // 使用默认参数初始化ZED相机
    return zedCamera_.initialize(sl::RESOLUTION::AUTO, 30, sl::DEPTH_MODE::NEURAL);
}

void ZEDCameraAdapter::closeCamera() {
    zedCamera_.close();
}

bool ZEDCameraAdapter::getFrame(cv::Mat& frame) {
    // 抓取一帧
    sl::ERROR_CODE returned_state = zedCamera_.grab();
    if (returned_state != sl::ERROR_CODE::SUCCESS) {
        return false;
    }
    
    // 获取左眼图像作为主图像
    sl::Mat zedImage;
    if (!zedCamera_.getLeftImage(zedImage)) {
        return false;
    }
    
    // 将ZED的Mat转换为OpenCV的Mat
    cv::Mat cvImage(zedImage.getHeight(), zedImage.getWidth(), CV_8UC4, zedImage.getPtr<sl::uchar1>(sl::MEM::CPU));
    cv::cvtColor(cvImage, frame, cv::COLOR_BGRA2BGR);
    
    return true;
}

bool ZEDCameraAdapter::isConnected() const {
    return zedCamera_.isConnected();
}

std::string ZEDCameraAdapter::getCameraName() const {
    return "ZED Camera";
}

bool ZEDCameraAdapter::getLeftImage(cv::Mat& image) {
    sl::Mat zedImage;
    if (!zedCamera_.getLeftImage(zedImage)) {
        return false;
    }
    
    cv::Mat cvImage(zedImage.getHeight(), zedImage.getWidth(), CV_8UC4, zedImage.getPtr<sl::uchar1>(sl::MEM::CPU));
    cv::cvtColor(cvImage, image, cv::COLOR_BGRA2BGR);
    return true;
}

bool ZEDCameraAdapter::getRightImage(cv::Mat& image) {
    sl::Mat zedImage;
    if (!zedCamera_.getRightImage(zedImage)) {
        return false;
    }
    
    cv::Mat cvImage(zedImage.getHeight(), zedImage.getWidth(), CV_8UC4, zedImage.getPtr<sl::uchar1>(sl::MEM::CPU));
    cv::cvtColor(cvImage, image, cv::COLOR_BGRA2BGR);
    return true;
}

bool ZEDCameraAdapter::getDepthMap(cv::Mat& depth) {
    sl::Mat zedDepth;
    if (!zedCamera_.getDepthMap(zedDepth)) {
        return false;
    }
    
    // 将深度图转换为OpenCV格式
    cv::Mat cvDepth(zedDepth.getHeight(), zedDepth.getWidth(), CV_32FC1, zedDepth.getPtr<sl::uchar1>(sl::MEM::CPU));
    cvDepth.copyTo(depth);
    return true;
}

bool ZEDCameraAdapter::getPointCloud(cv::Mat& point_cloud) {
    sl::Mat zedPointCloud;
    if (!zedCamera_.getPointCloud(zedPointCloud)) {
        return false;
    }
    
    // 将点云转换为OpenCV格式
    cv::Mat cvPointCloud(zedPointCloud.getHeight(), zedPointCloud.getWidth(), CV_32FC4, zedPointCloud.getPtr<sl::uchar1>(sl::MEM::CPU));
    cvPointCloud.copyTo(point_cloud);
    return true;
}

bool ZEDCameraAdapter::getPosition(sl::Pose& pose) {
    return zedCamera_.getPosition(pose);
}

bool ZEDCameraAdapter::getSensorsData(sl::SensorsData& sensor_data) {
    return zedCamera_.getSensorsData(sensor_data);
}

bool ZEDCameraAdapter::enablePositionalTracking() {
    return zedCamera_.enablePositionalTracking();
}

void ZEDCameraAdapter::disablePositionalTracking() {
    zedCamera_.disablePositionalTracking();
}

bool ZEDCameraAdapter::isPositionalTrackingEnabled() const {
    return zedCamera_.isPositionalTrackingEnabled();
}

bool ZEDCameraAdapter::hasIMU() const {
    return zedCamera_.hasIMU();
}

#endif // ZED_AVAILABLE 