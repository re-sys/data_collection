#include "camera_factory.hpp"
#include "hik_camera_adapter.hpp"
#include "zed_camera_adapter.hpp"
#include <algorithm>
#include <iostream>

std::unique_ptr<CameraInterface> CameraFactory::createCamera(CameraType type) {
    switch (type) {
        case CameraType::HIKVISION:
            return std::make_unique<HikCameraAdapter>();
        case CameraType::ZED:
#ifdef ZED_AVAILABLE
            return std::make_unique<ZEDCameraAdapter>();
#else
            std::cerr << "ZED camera requested but ZED SDK is not available!" << std::endl;
            return nullptr;
#endif
        default:
            std::cerr << "Unknown camera type!" << std::endl;
            return nullptr;
    }
}

std::unique_ptr<CameraInterface> CameraFactory::createCamera(const std::string& typeStr) {
    std::string lowerType = typeStr;
    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
    
    if (lowerType == "hikvision" || lowerType == "hik" || lowerType == "hikvision_camera") {
        return createCamera(CameraType::HIKVISION);
    } else if (lowerType == "zed" || lowerType == "zed_camera") {
#ifdef ZED_AVAILABLE
        return createCamera(CameraType::ZED);
#else
        std::cerr << "ZED camera requested but ZED SDK is not available!" << std::endl;
        std::cout << "Available camera types: hikvision" << std::endl;
        return nullptr;
#endif
    } else {
        std::cerr << "Unknown camera type: " << typeStr << std::endl;
        std::cout << "Supported camera types: ";
        auto supportedTypes = CameraFactory::getSupportedCameraTypes();
        for (const auto& type : supportedTypes) {
            std::cout << type << " ";
        }
        std::cout << std::endl;
        return nullptr;
    }
}

std::vector<std::string> CameraFactory::getSupportedCameraTypes() {
    std::vector<std::string> types = {"hikvision"};
#ifdef ZED_AVAILABLE
    types.push_back("zed");
#endif
    return types;
} 