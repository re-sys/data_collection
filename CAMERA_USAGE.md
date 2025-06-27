# 相机驱动选择功能使用说明

## 概述

本项目现在支持多种相机驱动，包括：
- **海康相机 (HikVision)**: 工业相机，支持高帧率和精确控制
- **ZED相机**: 立体相机，支持深度感知和位置跟踪

## 使用方法

### 1. 通过配置文件选择相机

在配置文件中添加 `camera_type` 参数：

```yaml
# 相机类型配置
# 可选值: "hikvision" (海康相机), "zed" (ZED相机)
camera_type: "hikvision"
```

然后使用 `camera` 模式运行：

```bash
./trajectory_solver camera config/example_config.yaml
```

### 2. 通过命令行强制指定相机类型

#### 使用海康相机
```bash
./trajectory_solver camera_hik config/example_config.yaml
```

#### 使用ZED相机
```bash
./trajectory_solver camera_zed config/example_config.yaml
```

### 3. 所有可用的运行模式

```bash
# 视频文件处理
./trajectory_solver video myvideo.mp4 config.yaml

# 单张图像处理
./trajectory_solver image img.png config.yaml

# 使用配置文件中的相机类型
./trajectory_solver camera config.yaml

# 强制使用海康相机
./trajectory_solver camera_hik config.yaml

# 强制使用ZED相机
./trajectory_solver camera_zed config.yaml
```

## 相机特性对比

| 特性 | 海康相机 | ZED相机 |
|------|----------|---------|
| 图像质量 | 高分辨率，低噪声 | 立体图像，深度信息 |
| 帧率 | 高帧率 (可达100+ fps) | 标准帧率 (30-60 fps) |
| 深度感知 | 无 | 支持深度图和点云 |
| 位置跟踪 | 无 | 支持SLAM位置跟踪 |
| IMU传感器 | 无 | 支持IMU数据 |
| 安装复杂度 | 简单 | 需要立体标定 |
| 价格 | 中等 | 较高 |

## 配置示例

### 海康相机配置
```yaml
camera_type: "hikvision"
# 其他参数...
```

### ZED相机配置
```yaml
camera_type: "zed"
# 其他参数...
```

## 编译要求

### 海康相机
- 安装海康MVS SDK
- 确保 `/opt/MVS/lib/64/libMvCameraControl.so` 存在

### ZED相机
- 安装ZED SDK
- 确保CMake能找到ZED SDK

## 故障排除

### 海康相机问题
1. 确保相机已正确连接
2. 检查MVS SDK是否正确安装
3. 确认相机权限设置

### ZED相机问题
1. 确保ZED SDK已正确安装
2. 检查相机USB连接
3. 确认ZED相机驱动已加载

### 编译问题
1. 检查CMakeLists.txt中的库路径
2. 确认所有依赖库都已安装
3. 检查编译器版本兼容性

## 高级功能

### ZED相机特有功能
如果使用ZED相机，还可以访问以下功能：
- 深度图获取
- 点云数据
- 位置跟踪
- IMU传感器数据

这些功能可以通过 `ZEDCameraAdapter` 类访问。

### 扩展新的相机驱动
要添加新的相机驱动，需要：
1. 实现 `CameraInterface` 接口
2. 在 `CameraFactory` 中注册新驱动
3. 更新CMakeLists.txt
4. 添加相应的配置选项 