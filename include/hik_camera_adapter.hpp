#pragma once

#include "camera_interface.hpp"
#include "hik_camera.hpp"

/**
 * @brief HikCamera适配器
 * 
 * 将HikCamera适配到CameraInterface接口
 */
class HikCameraAdapter : public CameraInterface {
public:
    HikCameraAdapter();
    ~HikCameraAdapter() override;
    
    bool openCamera() override;
    void closeCamera() override;
    bool getFrame(cv::Mat& frame) override;
    bool isConnected() const override;
    std::string getCameraName() const override;
    
    // 提供对原始HikCamera的访问（用于高级功能）
    HikCamera* getHikCamera() { return &hikCamera_; }
    const HikCamera* getHikCamera() const { return &hikCamera_; }

private:
    HikCamera hikCamera_;
}; 