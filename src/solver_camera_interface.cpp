#include "solver.hpp"
#include "camera_interface.hpp"
#include "camera_factory.hpp"
#include "basketball_detector.hpp"
#include "zed_camera_adapter.hpp"

#include <opencv2/aruco.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <deque>
#include <fstream>
#include <ctime>
#include <filesystem>

using namespace cv;
using namespace std;
using namespace std::chrono;

// 定义一组不同的轨迹颜色
const std::vector<cv::Scalar> TRAJECTORY_COLORS = {
    cv::Scalar(0, 255, 255),   // 黄色
    cv::Scalar(255, 0, 255),   // 洋红
    cv::Scalar(0, 255, 0),     // 绿色
    cv::Scalar(255, 128, 0),   // 橙色
    cv::Scalar(255, 255, 0),   // 青色
    cv::Scalar(128, 0, 255)    // 紫色
};

// 像素坐标 -> 相机坐标系射线
static cv::Vec3d pixelToCameraRay(const cv::Point2f &uv, const cv::Mat &K)
{
    double fx = K.at<double>(0, 0);
    double fy = K.at<double>(1, 1);
    double cx = K.at<double>(0, 2);
    double cy = K.at<double>(1, 2);
    cv::Vec3d dir((uv.x - cx) / fx, (uv.y - cy) / fy, 1.0);
    return cv::normalize(dir);
}

// 射线与平面求交
static bool intersectRayPlane(const cv::Vec3d &origin, const cv::Vec3d &dir,
                              const ::Plane &plane, cv::Vec3d &intersection)
{
    double denom = plane.normal.dot(dir);
    if (fabs(denom) < 1e-6) return false;
    double t = plane.normal.dot(plane.point - origin) / denom;
    if (t < 0) return false;
    intersection = origin + t * dir;
    return true;
}

//================= 相机接口版本 =================//
void runTrajectorySolverCameraInterface(const Config &cfg)
{
    // 根据配置创建相机
    auto camera = CameraFactory::createCamera(cfg.cameraType);
    if (!camera) {
        std::cerr << "[ERROR] Failed to create camera of type: " << cfg.cameraType << std::endl;
        return;
    }
    
    std::cout << "[INFO] Using camera: " << camera->getCameraName() << std::endl;
    
    if (!camera->openCamera()) {
        std::cerr << "[ERROR] Failed to open camera" << std::endl;
        return;
    }

    cv::Mat frame, undistorted;
    int frameIdx = 0;
    
    // 记录相关
    std::ofstream recordFile;
    bool recording = false;
    int sessionIndex = 0;

    // 用于控制日志输出频率
    auto lastLogTime = steady_clock::now();
    const auto logInterval = milliseconds(500);
    bool shouldLog = false;

    // 检测功能开关
    bool detectEnabled = true;
    const string windowName = "Basketball Detection";
    cv::namedWindow(windowName);
    const string binaryWindowName = "Binary Process";
    cv::namedWindow(binaryWindowName);

    // 存储轨迹点和对应的颜色索引
    struct TrajectorySegment {
        std::deque<cv::Point2f> points;
        size_t colorIndex;
    };
    std::vector<TrajectorySegment> trajectorySegments;
    trajectorySegments.push_back({std::deque<cv::Point2f>(), 0});
    const size_t maxTrajectoryPoints = 1000;

    // 添加面积阈值
    const double MIN_CONTOUR_AREA = 300.0;

    // 创建形态学操作的核
    cv::Mat morphKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    
    // 创建一个用于显示处理过程的图像
    cv::Mat processViz;

    while (true)
    {
        if (!camera->getFrame(frame)) {
            std::cerr << "[WARN] Failed to get camera frame, retrying..." << std::endl;
            continue;
        }
        if (frame.empty()) {
            std::cerr << "[WARN] Captured frame is empty, skipping this frame." << std::endl;
            continue;
        }
        // 检查ZED相机健康状态
        auto zedAdapter = dynamic_cast<ZEDCameraAdapter*>(camera.get());
        if (zedAdapter) {
            auto health = zedAdapter->getZEDCamera()->getHealthStatus();
            if (!health.enabled) {
                std::cerr << "[WARN] ZED camera health check failed, skipping this frame." << std::endl;
                continue;
            }
        }
        ++frameIdx;
        
        auto now = steady_clock::now();
        if (now - lastLogTime >= logInterval) {
            shouldLog = true;
            lastLogTime = now;
        } else {
            shouldLog = false;
        }

        cv::undistort(frame, undistorted, cfg.K, cfg.distCoeffs);

        // 显示检测状态
        string statusText = detectEnabled ? "Detection: ON [Space to toggle]" : "Detection: OFF [Space to toggle]";
        cv::putText(undistorted, statusText, 
                   cv::Point(15, undistorted.rows - 90),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, 
                   detectEnabled ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255), 2);

        // 显示清除轨迹的提示
        cv::putText(undistorted, "Press 'C' to clear trajectory", 
                   cv::Point(15, undistorted.rows - 120),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, 
                   cv::Scalar(255, 255, 255), 2);

        // 显示相机类型
        cv::putText(undistorted, "Camera: " + camera->getCameraName(), 
                   cv::Point(15, undistorted.rows - 150),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, 
                   cv::Scalar(255, 255, 0), 2);

        // 持续进行ArUco检测
        std::vector<int> markerIds;
        std::vector<std::vector<cv::Point2f>> corners;
        cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cfg.arucoDictId);
        cv::Ptr<cv::aruco::DetectorParameters> detectorParams = cv::aruco::DetectorParameters::create();
        cv::aruco::detectMarkers(undistorted, dictionary, corners, markerIds, detectorParams);

        bool hasValidAruco = false;
        std::vector<cv::Vec3d> rvecs, tvecs;
        
        if (!markerIds.empty()) {
            hasValidAruco = true;
            cv::aruco::estimatePoseSingleMarkers(corners, cfg.arucoMarkerLength, cfg.K, cfg.distCoeffs, rvecs, tvecs);
            
            // 绘制检测到的ArUco标记
            cv::aruco::drawDetectedMarkers(undistorted, corners, markerIds);
            
            for (size_t i = 0; i < markerIds.size(); ++i) {
                // 绘制坐标轴
                cv::aruco::drawAxis(undistorted, cfg.K, cfg.distCoeffs, rvecs[i], tvecs[i], cfg.arucoMarkerLength);
                
                // 在标记上显示ID
                cv::putText(undistorted, 
                          cv::format("ArUco ID: %d", markerIds[i]), 
                          corners[i][0], 
                          cv::FONT_HERSHEY_SIMPLEX, 0.6, 
                          cv::Scalar(0,255,255), 2);

                // 显示ArUco标记的3D位置
                cv::Vec3d tvec = tvecs[i];
                cv::putText(undistorted,
                          cv::format("ArUco %d Pos: (%.2f, %.2f, %.2f)m", 
                                   markerIds[i], tvec[0], tvec[1], tvec[2]),
                          cv::Point(15, 30 + i * 30),
                          cv::FONT_HERSHEY_SIMPLEX, 0.6,
                          cv::Scalar(255,255,0), 2);
            }
        }

        // 篮球检测
        bool hasValidBall = false;
        cv::Point2f center2D;
        
        BasketballDetector detector;
        detector.setHSVRange(cfg.hsvLow, cfg.hsvHigh);
        auto ballResult = detector.detect(undistorted);
        
        if (ballResult.mask.empty()) {
            std::cerr << "[WARN] ballResult.mask is empty, skipping this frame." << std::endl;
            continue;
        }
        // 创建可视化图像
        const int vizWidth = ballResult.mask.cols * 2;
        const int vizHeight = ballResult.mask.rows;
        processViz = cv::Mat::zeros(vizHeight, vizWidth, CV_8UC3);
        // 转换掩码为彩色图像以便显示
        cv::Mat maskViz, morphedViz;
        cv::cvtColor(ballResult.mask, maskViz, cv::COLOR_GRAY2BGR);
        if (maskViz.empty()) {
            std::cerr << "[WARN] maskViz is empty, skipping this frame." << std::endl;
            continue;
        }
        
        // 如果检测到篮球，显示检测结果
        if (ballResult.found) {
            cv::cvtColor(ballResult.mask, morphedViz, cv::COLOR_GRAY2BGR);
            if (morphedViz.empty()) {
                std::cerr << "[WARN] morphedViz is empty, skipping this frame." << std::endl;
                continue;
            }
            // 在掩码上绘制检测结果
            for (const auto &p : ballResult.inliers) {
                cv::circle(morphedViz, p, 2, cv::Scalar(255,0,255), -1);
            }
            
            // 绘制检测到的篮球轮廓
            cv::circle(undistorted, ballResult.center, ballResult.radius, cv::Scalar(0,255,0), 2);
            cv::circle(undistorted, ballResult.center, 3, cv::Scalar(0,255,0), -1);
            
            center2D = ballResult.center;
            hasValidBall = true;
            
            // 显示检测信息
            cv::putText(undistorted, 
                      cv::format("Ball: (%.1f, %.1f) R=%.1f Inliers=%zu", 
                               center2D.x, center2D.y, ballResult.radius, ballResult.inliers.size()),
                      cv::Point(15, undistorted.rows - 30),
                      cv::FONT_HERSHEY_SIMPLEX, 0.6, 
                      cv::Scalar(0,255,0), 2);
        } else {
            cv::cvtColor(ballResult.mask, morphedViz, cv::COLOR_GRAY2BGR);
            if (morphedViz.empty()) {
                std::cerr << "[WARN] morphedViz is empty, skipping this frame." << std::endl;
                continue;
            }
        }

        // 组合处理过程可视化
        cv::Mat leftViz, rightViz;
        cv::resize(maskViz, leftViz, cv::Size(vizWidth/2, vizHeight));
        cv::resize(morphedViz, rightViz, cv::Size(vizWidth/2, vizHeight));
        
        leftViz.copyTo(processViz(cv::Rect(0, 0, vizWidth/2, vizHeight)));
        rightViz.copyTo(processViz(cv::Rect(vizWidth/2, 0, vizWidth/2, vizHeight)));
        
        // 添加标签
        cv::putText(processViz, "Binary Mask", cv::Point(10, 30), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255,255,255), 2);
        cv::putText(processViz, "Morphed Result", cv::Point(vizWidth/2 + 10, 30), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255,255,255), 2);

        // 轨迹解算
        if (hasValidAruco && hasValidBall) {
            // 获取ArUco标记的旋转矩阵
            cv::Mat Rmat;
            cv::Rodrigues(rvecs[0], Rmat);
            cv::Matx33d R(Rmat);
            cv::Vec3d tvecVec(tvecs[0][0], tvecs[0][1], tvecs[0][2]);
            
            // 计算相机在世界坐标系中的位置
            cv::Vec3d cameraPos = -R.t() * tvecVec;
            
            // 计算篮球在相机坐标系中的射线方向
            cv::Vec3d rayDir = pixelToCameraRay(center2D, cfg.K);
            
            // 计算篮球在世界坐标系中的位置
            cv::Vec3d ballPos3D;
            if (intersectRayPlane(cameraPos, R.t() * rayDir, cfg.motionPlane, ballPos3D)) {
                // 应用原点偏移
                ballPos3D += cfg.originOffset;
                
                // 显示3D位置信息
                cv::putText(undistorted,
                          cv::format("3D Pos: (%.3f, %.3f, %.3f)m", 
                                   ballPos3D[0], ballPos3D[1], ballPos3D[2]),
                          cv::Point(15, undistorted.rows - 60),
                          cv::FONT_HERSHEY_SIMPLEX, 0.6,
                          cv::Scalar(255,255,255), 2);
                
                // 添加到轨迹
                cv::Point2f trajectoryPoint(ballPos3D[0], ballPos3D[2]); // X-Z平面投影
                trajectorySegments.back().points.push_back(trajectoryPoint);
                
                // 限制轨迹点数量
                if (trajectorySegments.back().points.size() > maxTrajectoryPoints) {
                    trajectorySegments.back().points.pop_front();
                }
                
                // 绘制轨迹
                for (const auto& segment : trajectorySegments) {
                    if (segment.points.size() < 2) continue;
                    
                    const auto& color = TRAJECTORY_COLORS[segment.colorIndex % TRAJECTORY_COLORS.size()];
                    
                    for (size_t i = 1; i < segment.points.size(); ++i) {
                        cv::Point2f p1 = segment.points[i-1];
                        cv::Point2f p2 = segment.points[i];
                        
                        // 将世界坐标转换为屏幕坐标（简单的缩放）
                        float scale = 100.0f; // 1米 = 100像素
                        cv::Point screenP1(p1.x * scale + undistorted.cols/2, 
                                         undistorted.rows/2 - p1.y * scale);
                        cv::Point screenP2(p2.x * scale + undistorted.cols/2, 
                                         undistorted.rows/2 - p2.y * scale);
                        
                        cv::line(undistorted, screenP1, screenP2, color, 2);
                    }
                }
                
                // 记录数据
                if (recording && recordFile.is_open()) {
                    auto now = std::chrono::system_clock::now();
                    auto time_t = std::chrono::system_clock::to_time_t(now);
                    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch()) % 1000;
                    
                    recordFile << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
                    recordFile << "." << std::setfill('0') << std::setw(3) << ms.count();
                    recordFile << "," << frameIdx;
                    recordFile << "," << ballPos3D[0] << "," << ballPos3D[1] << "," << ballPos3D[2];
                    recordFile << "," << center2D.x << "," << center2D.y;
                    recordFile << "," << ballResult.radius << "," << ballResult.inliers.size();
                    recordFile << "," << cfg.launchRPM;
                    recordFile << std::endl;
                }
            }
        }

        // 显示图像
        cv::imshow(windowName, undistorted);
        cv::imshow(binaryWindowName, processViz);

        // 键盘处理
        char key = cv::waitKey(1) & 0xFF;
        if (key == 27) { // ESC
            break;
        } else if (key == ' ') { // 空格键切换检测
            detectEnabled = !detectEnabled;
            std::cout << "Detection " << (detectEnabled ? "enabled" : "disabled") << std::endl;
        } else if (key == 'c' || key == 'C') { // 清除轨迹
            trajectorySegments.clear();
            trajectorySegments.push_back({std::deque<cv::Point2f>(), 0});
            std::cout << "Trajectory cleared" << std::endl;
        } else if (key == 'r' || key == 'R') { // 开始/停止记录
            if (!recording) {
                // 开始记录
                std::string filename = cfg.recordDir + "/trajectory_" + 
                                     std::to_string(sessionIndex++) + ".csv";
                recordFile.open(filename);
                if (recordFile.is_open()) {
                    recordFile << "Timestamp,Frame,X,Y,Z,PixelX,PixelY,Radius,Inliers,RPM\n";
                    recording = true;
                    std::cout << "Recording started: " << filename << std::endl;
                } else {
                    std::cerr << "Failed to open record file: " << filename << std::endl;
                }
            } else {
                // 停止记录
                recordFile.close();
                recording = false;
                std::cout << "Recording stopped" << std::endl;
            }
        }
    }

    // 清理
    if (recording) {
        recordFile.close();
    }
    camera->closeCamera();
    cv::destroyAllWindows();
}

void runTrajectorySolverCameraType(const std::string &cameraType, const Config &cfg) {
    // 创建临时配置
    Config tempCfg = cfg;
    tempCfg.cameraType = cameraType;
    
    // 调用相机接口版本
    runTrajectorySolverCameraInterface(tempCfg);
} 