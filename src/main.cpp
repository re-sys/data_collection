#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <iostream>
#include <iomanip>
#include <string>
#include "common.hpp"
#include "solver.hpp"
#include <vector>
#include "utils.hpp"
#include <filesystem>
namespace fs = std::filesystem;

using namespace std;

int main(int argc, char **argv)
{
    // 基本用法提示
    const std::string usage =
        std::string("用法: ") + argv[0] + " <mode> [path] [config.yaml]\n" +
        "  mode: video | image | camera | camera_zed | camera_hik\n" +
        "  当 mode 为 video 时, [path] 为视频文件路径\n" +
        "  当 mode 为 image 时, [path] 为图片文件路径\n" +
        "  当 mode 为 camera 时, 使用配置文件中的相机类型\n" +
        "  当 mode 为 camera_zed 时, 强制使用ZED相机\n" +
        "  当 mode 为 camera_hik 时, 强制使用海康相机\n" +
        "示例:\n" +
        "  " + argv[0] + " video myvideo.mp4 config.yaml\n" +
        "  " + argv[0] + " image img.png\n" +
        "  " + argv[0] + " camera config.yaml\n" +
        "  " + argv[0] + " camera_zed config.yaml\n" +
        "  " + argv[0] + " camera_hik config.yaml";

    // 至少需要一个 mode 参数
    if (argc < 2)
    {
        cerr << usage << endl;
        return -1;
    }

    std::string mode = argv[1];
    std::string inputPath;          // 对 camera 模式可为空
    std::string configPath;

    if (mode == "camera" || mode == "camera_zed" || mode == "camera_hik")
    {
        // camera 模式下, 第 2 个参数若存在则视为 config.yaml
        if (argc >= 3)
            configPath = argv[2];
    }
    else
    {
        // video / image 模式下必须提供路径
        if (argc < 3)
        {
            cerr << usage << endl;
            return -1;
        }
        inputPath = argv[2];
        if (argc >= 4)
            configPath = argv[3];
    }

    // 如果未显式给出 configPath, 尝试默认搜索路径
    if (configPath.empty())
    {
        std::vector<std::string> candidates = {
            "../config/example_config.yaml",   // build 目录执行
            "config/example_config.yaml",      // 源码根目录执行
            "./example_config.yaml"            // 当前目录
        };

        for(const auto& path : candidates){
            if(fs::exists(path)){
                configPath = path;
                break;
            }
        }
    }

    cout << "加载配置文件: " << configPath << endl;

    Config cfg;
    if (!loadConfig(configPath, cfg))
        return -1;

    if (mode == "video")
    {
        runTrajectorySolver(inputPath, cfg);
    }
    else if (mode == "image")
    {
        runTrajectorySolverImage(inputPath, cfg);
    }
    else if (mode == "camera")
    {
        // 使用配置文件中的相机类型
        runTrajectorySolverCameraInterface(cfg);
    }
    else if (mode == "camera_zed")
    {
        // 强制使用ZED相机
        cout << "强制使用ZED相机" << endl;
        runTrajectorySolverCameraType("zed", cfg);
    }
    else if (mode == "camera_hik")
    {
        // 强制使用海康相机
        cout << "强制使用海康相机" << endl;
        runTrajectorySolverCameraType("hikvision", cfg);
    }
    else
    {
        cerr << "未知模式: " << mode << " (需要 video | image | camera | camera_zed | camera_hik)" << endl;
        return -1;
    }

    return 0;
} 