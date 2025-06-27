#include "hik_camera_adapter.hpp"

HikCameraAdapter::HikCameraAdapter() {
}

HikCameraAdapter::~HikCameraAdapter() {
    closeCamera();
}

bool HikCameraAdapter::openCamera() {
    return hikCamera_.openCamera();
}

void HikCameraAdapter::closeCamera() {
    hikCamera_.closeCamera();
}

bool HikCameraAdapter::getFrame(cv::Mat& frame) {
    return hikCamera_.getFrame(frame);
}

bool HikCameraAdapter::isConnected() const {
    // HikCamera没有isConnected方法，我们假设如果相机已打开就认为已连接
    // 这里需要根据实际的HikCamera实现来调整
    return true; // 临时实现，需要根据实际情况修改
}

std::string HikCameraAdapter::getCameraName() const {
    return "HikVision Camera";
} 