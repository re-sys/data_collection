cmake_minimum_required(VERSION 3.10)

# 项目名称
project(BasketballTrajectorySolver LANGUAGES CXX)

# C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 OpenCV
find_package(OpenCV REQUIRED core calib3d imgproc highgui aruco video)

# 如果未找到 OpenCV，则给出提示
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found. Please install OpenCV and set OpenCV_DIR.")
endif()

# 配置 ZED SDK 路径
set(ZED_INCLUDE_DIR "/usr/local/zed/include")
set(ZED_LIBRARY_DIR "/usr/local/zed/lib")

# 检查 ZED SDK 是否存在
if(NOT EXISTS "${ZED_INCLUDE_DIR}/sl/Camera.hpp")
    message(WARNING "ZED SDK not found at ${ZED_INCLUDE_DIR}. ZED camera support will be disabled.")
    set(ZED_AVAILABLE FALSE)
else()
    message(STATUS "Found ZED SDK at ${ZED_INCLUDE_DIR}")
    set(ZED_AVAILABLE TRUE)
    add_definitions(-DZED_AVAILABLE)
endif()

# 公共源文件 (通用配置加载等)
set(COMMON_SRC src/utils.cpp)

# 新增: 海康相机源码
set(HIK_CAMERA_SRC src/hik_camera.cpp)

# 新增: 相机接口源码
set(CAMERA_INTERFACE_SRC 
    src/camera_factory.cpp
    src/hik_camera_adapter.cpp
    src/solver_camera_interface.cpp
)

# 新增: ZED相机源码 (条件编译)
if(ZED_AVAILABLE)
    set(CAMERA_INTERFACE_SRC ${CAMERA_INTERFACE_SRC} src/zed_camera_adapter.cpp)
    set(ZED_CAMERA_SRC src/zed_camera/zed_camera.cpp)
else()
    # 如果ZED不可用，创建一个空的适配器
    set(ZED_CAMERA_SRC "")
endif()

# 头文件目录
include_directories(${CMAKE_SOURCE_DIR}/include)
# 新增: 海康 SDK 头文件路径
include_directories(/opt/MVS/include)

# 新增: ZED SDK 头文件路径
if(ZED_AVAILABLE)
    include_directories(${ZED_INCLUDE_DIR})
endif()

# 主轨迹求解器
add_executable(trajectory_solver 
    src/main.cpp 
    src/solver.cpp 
    src/solver_camera.cpp 
    src/basketball_detector.cpp 
    ${COMMON_SRC} 
    ${HIK_CAMERA_SRC}
    ${CAMERA_INTERFACE_SRC}
    ${ZED_CAMERA_SRC}
)

# 篮球检测调试器
add_executable(basketball_detect src/basketball_detect.cpp ${COMMON_SRC})

# 篮球识别与HSV校准工具
add_executable(basketball_tool src/basketball_tool.cpp src/basketball_detector.cpp ${COMMON_SRC} ${HIK_CAMERA_SRC})

# 链接 OpenCV、海康相机 SDK 和 ZED SDK
target_link_libraries(trajectory_solver PRIVATE 
    ${OpenCV_LIBS} 
    /opt/MVS/lib/64/libMvCameraControl.so
)

# 条件链接 ZED SDK
if(ZED_AVAILABLE)
    target_link_libraries(trajectory_solver PRIVATE 
        ${ZED_LIBRARY_DIR}/libsl_zed.so
        ${ZED_LIBRARY_DIR}/libsl_ai.so
    )
    target_link_directories(trajectory_solver PRIVATE ${ZED_LIBRARY_DIR})
endif()

target_link_libraries(basketball_detect PRIVATE ${OpenCV_LIBS})
target_link_libraries(basketball_tool PRIVATE 
    ${OpenCV_LIBS} 
    /opt/MVS/lib/64/libMvCameraControl.so
) 