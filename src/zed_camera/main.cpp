#include "zed_camera.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

int main() {
    ZEDCamera camera;
    
    // 先尝试较低分辨率测试
    std::cout << "Testing different resolutions for optimal FPS..." << std::endl;
    
    // 测试HD1080分辨率，启用图像质量检测
    if (!camera.initialize(RESOLUTION::HD1080, 60, DEPTH_MODE::NONE)) {
        std::cout << "Failed to initialize camera with HD1080!" << std::endl;
        return EXIT_FAILURE;
    }

    // 获取相机信息
    auto camera_info = camera.getCameraInfo();
    int actual_width = camera_info.camera_configuration.resolution.width;
    int actual_height = camera_info.camera_configuration.resolution.height;
    float actual_fps = camera_info.camera_configuration.fps;
    
    std::cout << "Camera Model: " << camera_info.camera_model << std::endl;
    std::cout << "Serial Number: " << camera_info.serial_number << std::endl;
    std::cout << "Actual Resolution: " << actual_width << "x" << actual_height << std::endl;
    std::cout << "Actual FPS: " << actual_fps << std::endl;
    std::cout << "Press ESC to exit, R to start/stop recording" << std::endl;

    // 固定窗口大小
    const int win_width = 1280;
    const int win_height = 720;
    cv::namedWindow("ZED Left Image", cv::WINDOW_NORMAL);
    cv::resizeWindow("ZED Left Image", win_width, win_height);

    sl::Mat left_image;
    
    // 帧率计算变量
    auto start_time = std::chrono::high_resolution_clock::now();
    int frame_count = 0;
    double fps = 0.0;
    
    // 录像相关变量
    cv::VideoWriter video_writer;
    bool is_recording = false;
    std::string current_video_filename;
    int recording_frame_count = 0;
    
    // 图像质量检测相关变量
    int total_frames = 0;
    int corrupted_frames = 0;
    int low_quality_frames = 0;
    int low_lighting_frames = 0;
    int skipped_frames = 0;
    double corrupted_ratio = 0.0;
    
    // 生成时间戳字符串的函数
    auto get_timestamp_string = []() -> std::string {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
        return oss.str();
    };
    
    while (true) {
        // 抓取图像并检查质量
        auto grab_result = camera.grab();
        total_frames++;
        
        // 检查图像质量
        bool is_corrupted = false;
        bool is_low_quality = false;
        bool is_low_lighting = false;
        
        if (grab_result == sl::ERROR_CODE::CORRUPTED_FRAME) {
            corrupted_frames++;
            is_corrupted = true;
            std::cout << "*** CORRUPTED FRAME DETECTED! ***" << std::endl;
        } else if (grab_result != sl::ERROR_CODE::SUCCESS) {
            std::cout << "Error during capture: " << grab_result << std::endl;
            if (grab_result != sl::ERROR_CODE::CAMERA_REBOOTING) {
                break;
            }
            continue;
        }
        
        // 获取健康状态
        auto health = camera.getHealthStatus();
        if (health.low_image_quality) {
            low_quality_frames++;
            is_low_quality = true;
        }
        if (health.low_lighting) {
            low_lighting_frames++;
            is_low_lighting = true;
        }
        
        // 计算损坏帧比例
        corrupted_ratio = (double)corrupted_frames / total_frames;
        
        // 如果图像质量太差，跳过这一帧
        if (is_corrupted || is_low_quality) {
            skipped_frames++;
            continue;
        }
        
        // 获取图像
        if (!camera.getLeftImage(left_image)) {
            continue;
        }
        
        frame_count++;
        
        // 计算帧率
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
        if (elapsed >= 1000) { // 每秒更新一次帧率
            fps = frame_count * 1000.0 / elapsed;
            frame_count = 0;
            start_time = current_time;
        }
        
        // 将sl::Mat转为cv::Mat
        cv::Mat cv_img(left_image.getHeight(), left_image.getWidth(), CV_8UC4, left_image.getPtr<sl::uchar1>(sl::MEM::CPU));
        // 转为BGR
        cv::Mat cv_img_bgr;
        cv::cvtColor(cv_img, cv_img_bgr, cv::COLOR_BGRA2BGR);
        
        // 缩放到固定窗口用于显示
        cv::Mat cv_img_resized;
        cv::resize(cv_img_bgr, cv_img_resized, cv::Size(win_width, win_height));
        
        // 处理按键输入
        int key = cv::waitKey(1);
        if (key == 27) break; // ESC键退出
        else if (key == 'r' || key == 'R') { // R键开始/停止录像
            if (!is_recording) {
                // 开始录像 - 使用原图分辨率
                std::string timestamp = get_timestamp_string();
                current_video_filename = "zed_recording_" + timestamp + ".mp4";
                
                // 使用MP4V编码器，保存1080p原图质量
                int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
                if (!video_writer.open(current_video_filename, fourcc, 30.0, cv::Size(actual_width, actual_height), true)) {
                    // 如果MP4V失败，尝试XVID
                    fourcc = cv::VideoWriter::fourcc('X', 'V', 'I', 'D');
                    current_video_filename = "zed_recording_" + timestamp + ".avi";
                    if (!video_writer.open(current_video_filename, fourcc, 30.0, cv::Size(actual_width, actual_height), true)) {
                        std::cout << "Failed to create video writer!" << std::endl;
                        continue;
                    }
                }
                
                is_recording = true;
                recording_frame_count = 0;
                std::cout << "Started recording: " << current_video_filename << " at " << actual_width << "x" << actual_height << std::endl;
            } else {
                // 停止录像
                video_writer.release();
                is_recording = false;
                std::cout << "Stopped recording: " << current_video_filename << std::endl;
                std::cout << "Recorded " << recording_frame_count << " frames" << std::endl;
            }
        }
        
        // 如果正在录像，写入原图质量的帧
        if (is_recording && video_writer.isOpened()) {
            video_writer.write(cv_img_bgr); // 使用原图质量，不缩放
            recording_frame_count++;
        }
        
        // 在图像上添加信息
        std::string info_text = "Resolution: " + std::to_string(actual_width) + "x" + std::to_string(actual_height) + 
                               " | FPS: " + std::to_string(static_cast<int>(fps)) + 
                               " | Window: " + std::to_string(win_width) + "x" + std::to_string(win_height);
        
        cv::putText(cv_img_resized, info_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        
        // 添加录像状态信息
        std::string recording_text = is_recording ? 
            "RECORDING: " + current_video_filename + " | Frames: " + std::to_string(recording_frame_count) + " | Quality: " + std::to_string(actual_width) + "x" + std::to_string(actual_height) :
            "Press R to start recording";
        cv::Scalar recording_color = is_recording ? cv::Scalar(0, 0, 255) : cv::Scalar(255, 255, 255);
        cv::putText(cv_img_resized, recording_text, cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.6, recording_color, 2);
        
        // 添加图像质量信息
        std::string quality_text = "Quality: Corrupted: " + std::to_string(corrupted_frames) + 
                                  " (" + std::to_string(static_cast<int>(corrupted_ratio * 100)) + "%)" +
                                  " | Low Quality: " + std::to_string(low_quality_frames) +
                                  " | Low Lighting: " + std::to_string(low_lighting_frames) +
                                  " | Skipped: " + std::to_string(skipped_frames);
        
        cv::Scalar quality_color = (corrupted_ratio > 0.1) ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 255);
        cv::putText(cv_img_resized, quality_text, cv::Point(10, 90), cv::FONT_HERSHEY_SIMPLEX, 0.5, quality_color, 1);
        
        // 如果当前帧有问题，显示警告
        if (is_low_lighting) {
            cv::putText(cv_img_resized, "LOW LIGHTING", cv::Point(10, 120), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 0, 255), 2);
        }
        
        cv::imshow("ZED Left Image", cv_img_resized);
    }
    
    // 确保录像器被正确关闭
    if (is_recording && video_writer.isOpened()) {
        video_writer.release();
        std::cout << "Recording stopped on exit" << std::endl;
    }
    
    // 打印最终统计信息
    std::cout << "\n=== Final Quality Statistics ===" << std::endl;
    std::cout << "Total frames processed: " << total_frames << std::endl;
    std::cout << "Corrupted frames: " << corrupted_frames << " (" << (corrupted_ratio * 100) << "%)" << std::endl;
    std::cout << "Low quality frames: " << low_quality_frames << std::endl;
    std::cout << "Low lighting frames: " << low_lighting_frames << std::endl;
    std::cout << "Skipped frames: " << skipped_frames << std::endl;
    std::cout << "================================" << std::endl;
    
    camera.close();
    return 0;
} 