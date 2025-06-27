#pragma once

#include "camera_interface.hpp"

// 检查ZED SDK是否可用
#ifdef ZED_AVAILABLE
#include "../src/zed_camera/zed_camera.h"

/**
 * @brief ZED相机适配器
 * 
 * 将ZEDCamera适配到CameraInterface接口
 */
class ZEDCameraAdapter : public CameraInterface {
public:
    ZEDCameraAdapter();
    ~ZEDCameraAdapter() override;
    
    bool openCamera() override;
    void closeCamera() override;
    bool getFrame(cv::Mat& frame) override;
    bool isConnected() const override;
    std::string getCameraName() const override;
    
    // 提供对原始ZEDCamera的访问（用于高级功能）
    ZEDCamera* getZEDCamera() { return &zedCamera_; }
    const ZEDCamera* getZEDCamera() const { return &zedCamera_; }
    
    // ZED相机特有的功能
    bool getLeftImage(cv::Mat& image);
    bool getRightImage(cv::Mat& image);
    bool getDepthMap(cv::Mat& depth);
    bool getPointCloud(cv::Mat& point_cloud);
    bool getPosition(sl::Pose& pose);
    bool getSensorsData(sl::SensorsData& sensor_data);
    bool enablePositionalTracking();
    void disablePositionalTracking();
    bool isPositionalTrackingEnabled() const;
    bool hasIMU() const;

private:
    ZEDCamera zedCamera_;
    cv::Mat currentFrame_;  // 缓存当前帧
};

#else

/**
 * @brief ZED相机适配器 (ZED SDK不可用时)
 * 
 * 当ZED SDK不可用时，提供一个空的适配器实现
 */
class ZEDCameraAdapter : public CameraInterface {
public:
    ZEDCameraAdapter() {}
    ~ZEDCameraAdapter() override {}
    
    bool openCamera() override { 
        std::cerr << "ZED SDK not available. Cannot open ZED camera." << std::endl;
        return false; 
    }
    void closeCamera() override {}
    bool getFrame(cv::Mat& frame) override { 
        std::cerr << "ZED SDK not available. Cannot get frame from ZED camera." << std::endl;
        return false; 
    }
    bool isConnected() const override { return false; }
    std::string getCameraName() const override { return "ZED Camera (SDK not available)"; }
    
    // 提供对原始ZEDCamera的访问（用于高级功能）
    void* getZEDCamera() { return nullptr; }
    const void* getZEDCamera() const { return nullptr; }
    
    // ZED相机特有的功能
    bool getLeftImage(cv::Mat& image) { return false; }
    bool getRightImage(cv::Mat& image) { return false; }
    bool getDepthMap(cv::Mat& depth) { return false; }
    bool getPointCloud(cv::Mat& point_cloud) { return false; }
    bool getPosition(void* pose) { return false; }
    bool getSensorsData(void* sensor_data) { return false; }
    bool enablePositionalTracking() { return false; }
    void disablePositionalTracking() {}
    bool isPositionalTrackingEnabled() const { return false; }
    bool hasIMU() const { return false; }
};

#endif 