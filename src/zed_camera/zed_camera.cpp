#include "zed_camera.h"
#include <iostream>
#include <cmath>

ZEDCamera::ZEDCamera() 
    : camera_(std::make_unique<Camera>())
    , is_initialized_(false)
    , tracking_enabled_(false) {
}

ZEDCamera::~ZEDCamera() {
    if (is_initialized_) {
        close();
    }
}

bool ZEDCamera::initialize(RESOLUTION resolution, int fps, DEPTH_MODE depth_mode) {
    if (is_initialized_) {
        std::cout << "Camera already initialized!" << std::endl;
        return true;
    }
    
    // 设置初始化参数
    init_params_.camera_resolution = resolution;
    init_params_.camera_fps = fps;
    init_params_.depth_mode = depth_mode;
    init_params_.coordinate_units = UNIT::METER;
    init_params_.coordinate_system = COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP;
    init_params_.sensors_required = true;
    
    // 启用图像质量检测
    // 1: 基本健康检查
    // 2: 图像质量处理（绝对质量和左右图像差异）
    // 3: 高级模糊和质量检查
    init_params_.enable_image_validity_check = 2;
    
    std::cout << "Enabling ZED image quality detection (level 2)..." << std::endl;
    
    // 打开相机
    ERROR_CODE returned_state = camera_->open(init_params_);
    if (returned_state != ERROR_CODE::SUCCESS) {
        std::cout << "Error " << returned_state << ", failed to open camera." << std::endl;
        return false;
    }
    
    // 获取相机信息
    camera_info_ = camera_->getCameraInformation();
    is_initialized_ = true;
    
    std::cout << "ZED Camera initialized successfully!" << std::endl;
    std::cout << "Serial number: " << camera_info_.serial_number << std::endl;
    std::cout << "Camera model: " << camera_info_.camera_model << std::endl;
    std::cout << "Firmware version: " << camera_info_.camera_configuration.firmware_version << std::endl;
    
    return true;
}

bool ZEDCamera::enablePositionalTracking() {
    if (!is_initialized_) {
        std::cout << "Camera not initialized!" << std::endl;
        return false;
    }
    
    if (tracking_enabled_) {
        std::cout << "Positional tracking already enabled!" << std::endl;
        return true;
    }
    
    ERROR_CODE returned_state = camera_->enablePositionalTracking(tracking_params_);
    if (returned_state != ERROR_CODE::SUCCESS) {
        std::cout << "Error " << returned_state << ", failed to enable positional tracking." << std::endl;
        return false;
    }
    
    tracking_enabled_ = true;
    std::cout << "Positional tracking enabled successfully!" << std::endl;
    return true;
}

void ZEDCamera::disablePositionalTracking() {
    if (tracking_enabled_) {
        camera_->disablePositionalTracking();
        tracking_enabled_ = false;
        std::cout << "Positional tracking disabled." << std::endl;
    }
}

CameraInformation ZEDCamera::getCameraInfo() const {
    return camera_info_;
}

ERROR_CODE ZEDCamera::grab() {
    if (!is_initialized_) {
        std::cout << "Camera not initialized!" << std::endl;
        return ERROR_CODE::CAMERA_NOT_INITIALIZED;
    }
    
    ERROR_CODE returned_state = camera_->grab();
    return returned_state;
}

bool ZEDCamera::getLeftImage(Mat& image) {
    if (!is_initialized_) {
        std::cout << "Camera not initialized!" << std::endl;
        return false;
    }
    
    ERROR_CODE returned_state = camera_->retrieveImage(image, VIEW::LEFT);
    return (returned_state == ERROR_CODE::SUCCESS);
}

bool ZEDCamera::getRightImage(Mat& image) {
    if (!is_initialized_) {
        std::cout << "Camera not initialized!" << std::endl;
        return false;
    }
    
    ERROR_CODE returned_state = camera_->retrieveImage(image, VIEW::RIGHT);
    return (returned_state == ERROR_CODE::SUCCESS);
}

bool ZEDCamera::getDepthMap(Mat& depth) {
    if (!is_initialized_) {
        std::cout << "Camera not initialized!" << std::endl;
        return false;
    }
    
    ERROR_CODE returned_state = camera_->retrieveMeasure(depth, MEASURE::DEPTH);
    return (returned_state == ERROR_CODE::SUCCESS);
}

bool ZEDCamera::getPointCloud(Mat& point_cloud) {
    if (!is_initialized_) {
        std::cout << "Camera not initialized!" << std::endl;
        return false;
    }
    
    ERROR_CODE returned_state = camera_->retrieveMeasure(point_cloud, MEASURE::XYZRGBA);
    return (returned_state == ERROR_CODE::SUCCESS);
}

bool ZEDCamera::getPosition(Pose& pose) {
    if (!is_initialized_ || !tracking_enabled_) {
        std::cout << "Camera not initialized or positional tracking not enabled!" << std::endl;
        return false;
    }
    
    POSITIONAL_TRACKING_STATE state = camera_->getPosition(pose, REFERENCE_FRAME::WORLD);
    return (state == POSITIONAL_TRACKING_STATE::OK);
}

bool ZEDCamera::getSensorsData(SensorsData& sensor_data) {
    if (!is_initialized_) {
        std::cout << "Camera not initialized!" << std::endl;
        return false;
    }
    
    ERROR_CODE returned_state = camera_->getSensorsData(sensor_data, TIME_REFERENCE::IMAGE);
    return (returned_state == ERROR_CODE::SUCCESS);
}

bool ZEDCamera::isConnected() const {
    return is_initialized_;
}

bool ZEDCamera::isPositionalTrackingEnabled() const {
    return tracking_enabled_;
}

bool ZEDCamera::hasIMU() const {
    if (!is_initialized_) {
        return false;
    }
    auto sensors_config = camera_info_.sensors_configuration;
    return sensors_config.isSensorAvailable(SENSOR_TYPE::GYROSCOPE);
}

void ZEDCamera::close() {
    if (tracking_enabled_) {
        disablePositionalTracking();
    }
    
    if (is_initialized_) {
        camera_->close();
        is_initialized_ = false;
        std::cout << "Camera closed." << std::endl;
    }
}

int ZEDCamera::getSerialNumber() const {
    return camera_info_.serial_number;
}

Resolution ZEDCamera::getResolution() const {
    return camera_info_.camera_configuration.resolution;
}

HealthStatus ZEDCamera::getHealthStatus() const {
    if (!is_initialized_) {
        HealthStatus status;
        status.enabled = false;
        return status;
    }
    
    return camera_->getHealthStatus();
} 