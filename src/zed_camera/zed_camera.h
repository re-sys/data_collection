#ifndef ZED_CAMERA_H
#define ZED_CAMERA_H

#include <sl/Camera.hpp>
#include <memory>
#include <string>

using namespace sl;

/**
 * @brief 简化的ZED相机驱动类
 * 
 * 这个类封装了ZED相机的基本功能，包括：
 * - 相机初始化和配置
 * - 图像捕获
 * - 深度感知
 * - 位置跟踪
 * - 传感器数据获取
 * - 图像质量检测
 */
class ZEDCamera {
public:
    /**
     * @brief 构造函数
     */
    ZEDCamera();
    
    /**
     * @brief 析构函数
     */
    ~ZEDCamera();
    
    /**
     * @brief 初始化相机
     * @param resolution 相机分辨率
     * @param fps 帧率
     * @param depth_mode 深度模式
     * @return 是否成功初始化
     */
    bool initialize(RESOLUTION resolution = RESOLUTION::AUTO, 
                   int fps = 30, 
                   DEPTH_MODE depth_mode = DEPTH_MODE::NEURAL);
    
    /**
     * @brief 启用位置跟踪
     * @return 是否成功启用
     */
    bool enablePositionalTracking();
    
    /**
     * @brief 禁用位置跟踪
     */
    void disablePositionalTracking();
    
    /**
     * @brief 获取相机信息
     * @return 相机信息结构体
     */
    CameraInformation getCameraInfo() const;
    
    /**
     * @brief 抓取一帧图像
     * @return 抓取结果（ERROR_CODE）
     */
    ERROR_CODE grab();
    
    /**
     * @brief 获取左眼图像
     * @param image 输出图像矩阵
     * @return 是否成功获取
     */
    bool getLeftImage(Mat& image);
    
    /**
     * @brief 获取右眼图像
     * @param image 输出图像矩阵
     * @return 是否成功获取
     */
    bool getRightImage(Mat& image);
    
    /**
     * @brief 获取深度图
     * @param depth 输出深度矩阵
     * @return 是否成功获取
     */
    bool getDepthMap(Mat& depth);
    
    /**
     * @brief 获取点云数据
     * @param point_cloud 输出点云矩阵
     * @return 是否成功获取
     */
    bool getPointCloud(Mat& point_cloud);
    
    /**
     * @brief 获取相机位置
     * @param pose 输出位置信息
     * @return 是否成功获取
     */
    bool getPosition(Pose& pose);
    
    /**
     * @brief 获取传感器数据
     * @param sensor_data 输出传感器数据
     * @return 是否成功获取
     */
    bool getSensorsData(SensorsData& sensor_data);
    
    /**
     * @brief 获取相机健康状态
     * @return 健康状态结构体
     */
    HealthStatus getHealthStatus() const;
    
    /**
     * @brief 检查相机是否已连接
     * @return 是否已连接
     */
    bool isConnected() const;
    
    /**
     * @brief 检查位置跟踪是否已启用
     * @return 是否已启用
     */
    bool isPositionalTrackingEnabled() const;
    
    /**
     * @brief 检查是否有IMU传感器
     * @return 是否有IMU
     */
    bool hasIMU() const;
    
    /**
     * @brief 关闭相机
     */
    void close();
    
    /**
     * @brief 获取相机序列号
     * @return 序列号
     */
    int getSerialNumber() const;
    
    /**
     * @brief 获取图像分辨率
     * @return 分辨率
     */
    Resolution getResolution() const;

private:
    std::unique_ptr<Camera> camera_;           ///< ZED相机对象
    InitParameters init_params_;               ///< 初始化参数
    PositionalTrackingParameters tracking_params_; ///< 位置跟踪参数
    bool is_initialized_;                      ///< 是否已初始化
    bool tracking_enabled_;                    ///< 位置跟踪是否已启用
    CameraInformation camera_info_;            ///< 相机信息
};

#endif // ZED_CAMERA_H 