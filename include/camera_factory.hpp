#pragma once

#include <memory>
#include <string>
#include <vector>
#include "camera_interface.hpp"

class CameraFactory {
public:
    static std::unique_ptr<CameraInterface> createCamera(CameraType type);
    static std::unique_ptr<CameraInterface> createCamera(const std::string& typeStr);
    static std::vector<std::string> getSupportedCameraTypes();
}; 