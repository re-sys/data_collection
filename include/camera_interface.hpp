#pragma once

#include <opencv2/opencv.hpp>
#include <string>

/**
 * @brief 相机驱动接口
 * 
 * 定义统一的相机操作接口，支持不同的相机驱动实现
 */
class CameraInterface {
public:
    virtual ~CameraInterface() = default;
    
    /**
     * @brief 打开相机
     * @return 是否成功打开
     */
    virtual bool openCamera() = 0;
    
    /**
     * @brief 关闭相机
     */
    virtual void closeCamera() = 0;
    
    /**
     * @brief 获取一帧图像
     * @param frame 输出图像
     * @return 是否成功获取
     */
    virtual bool getFrame(cv::Mat& frame) = 0;
    
    /**
     * @brief 检查相机是否已连接
     * @return 是否已连接
     */
    virtual bool isConnected() const = 0;
    
    /**
     * @brief 获取相机名称
     * @return 相机名称
     */
    virtual std::string getCameraName() const = 0;
};

/**
 * @brief 相机类型枚举
 */
enum class CameraType {
    HIKVISION,  // 海康相机
    ZED         // ZED相机
}; 