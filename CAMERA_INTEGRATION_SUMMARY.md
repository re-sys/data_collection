# 相机驱动选择功能集成总结

## 完成的工作

### 1. 创建相机接口架构

#### 核心接口文件
- **`include/camera_interface.hpp`**: 定义了统一的相机操作接口 `CameraInterface`
- **`include/camera_factory.hpp`**: 相机工厂类，用于创建不同类型的相机驱动

#### 相机类型枚举
```cpp
enum class CameraType {
    HIKVISION,  // 海康相机
    ZED         // ZED相机
};
```

### 2. 实现相机适配器

#### 海康相机适配器
- **`include/hik_camera_adapter.hpp`**: HikCamera适配器头文件
- **`src/hik_camera_adapter.cpp`**: 实现CameraInterface接口的HikCamera适配器

#### ZED相机适配器
- **`include/zed_camera_adapter.hpp`**: ZED相机适配器头文件
- **`src/zed_camera_adapter.cpp`**: 实现CameraInterface接口的ZED相机适配器

### 3. 实现相机工厂

#### 工厂类实现
- **`src/camera_factory.cpp`**: 实现相机工厂类，支持：
  - 通过枚举类型创建相机
  - 通过字符串创建相机
  - 获取支持的相机类型列表

#### 支持的相机类型字符串
- `"hikvision"`, `"hik"`, `"hikvision_camera"` → 海康相机
- `"zed"`, `"zed_camera"` → ZED相机

### 4. 更新配置系统

#### 配置文件支持
- 在 `Config` 结构体中添加 `cameraType` 字段
- 更新 `loadConfig` 函数，支持从YAML文件加载相机类型
- 更新示例配置文件 `config/example_config.yaml`

#### 配置示例
```yaml
# 相机类型配置
camera_type: "hikvision"  # 或 "zed"
```

### 5. 创建新的Solver实现

#### 相机接口版本
- **`src/solver_camera_interface.cpp`**: 使用相机接口的新solver实现
- 支持动态选择相机类型
- 保持与原有功能的兼容性

#### 新增函数
```cpp
// 使用相机接口的solver
void runTrajectorySolverCameraInterface(const Config &cfg);

// 指定相机类型的solver
void runTrajectorySolverCameraType(const std::string &cameraType, const Config &cfg);
```

### 6. 更新主程序

#### 命令行支持
- 更新 `src/main.cpp`，添加新的运行模式：
  - `camera`: 使用配置文件中的相机类型
  - `camera_hik`: 强制使用海康相机
  - `camera_zed`: 强制使用ZED相机

#### 使用示例
```bash
# 使用配置文件中的相机类型
./trajectory_solver camera config.yaml

# 强制使用海康相机
./trajectory_solver camera_hik config.yaml

# 强制使用ZED相机
./trajectory_solver camera_zed config.yaml
```

### 7. 更新构建系统

#### CMakeLists.txt更新
- 添加新的源文件到构建系统
- 添加ZED SDK依赖
- 更新链接库配置

#### 新增源文件
```cmake
set(CAMERA_INTERFACE_SRC 
    src/camera_factory.cpp
    src/hik_camera_adapter.cpp
    src/zed_camera_adapter.cpp
    src/solver_camera_interface.cpp
)

set(ZED_CAMERA_SRC src/zed_camera/zed_camera.cpp)
```

### 8. 创建文档和测试

#### 使用文档
- **`CAMERA_USAGE.md`**: 详细的使用说明文档
- **`CAMERA_INTEGRATION_SUMMARY.md`**: 本总结文档

#### 测试脚本
- **`scripts/test_camera_selection.sh`**: 测试相机选择功能的脚本

## 架构设计

### 设计模式
- **工厂模式**: 使用 `CameraFactory` 创建相机实例
- **适配器模式**: 将现有相机驱动适配到统一接口
- **策略模式**: 通过配置选择不同的相机驱动

### 接口设计
```cpp
class CameraInterface {
public:
    virtual ~CameraInterface() = default;
    virtual bool openCamera() = 0;
    virtual void closeCamera() = 0;
    virtual bool getFrame(cv::Mat& frame) = 0;
    virtual bool isConnected() const = 0;
    virtual std::string getCameraName() const = 0;
};
```

### 扩展性
- 易于添加新的相机驱动
- 保持向后兼容性
- 支持高级功能的访问

## 使用方法

### 1. 编译项目
```bash
cd build
cmake ..
make
```

### 2. 配置相机类型
在配置文件中设置：
```yaml
camera_type: "hikvision"  # 或 "zed"
```

### 3. 运行程序
```bash
# 使用配置文件中的相机类型
./trajectory_solver camera config.yaml

# 强制指定相机类型
./trajectory_solver camera_hik config.yaml
./trajectory_solver camera_zed config.yaml
```

## 优势

### 1. 统一接口
- 所有相机驱动使用相同的接口
- 简化了solver的实现
- 提高了代码的可维护性

### 2. 灵活配置
- 支持通过配置文件选择相机
- 支持通过命令行强制指定相机
- 支持运行时动态选择

### 3. 易于扩展
- 添加新相机驱动只需实现接口
- 不需要修改现有代码
- 支持高级功能的访问

### 4. 向后兼容
- 保持原有功能的完整性
- 不影响现有的使用方式
- 平滑的升级路径

## 注意事项

### 1. 依赖要求
- 海康相机需要MVS SDK
- ZED相机需要ZED SDK
- 确保所有依赖库正确安装

### 2. 编译配置
- 检查CMakeLists.txt中的库路径
- 确保编译器版本兼容
- 验证所有依赖库链接正确

### 3. 运行时配置
- 确保相机硬件正确连接
- 检查相机权限设置
- 验证配置文件格式正确

## 未来扩展

### 1. 支持更多相机类型
- USB相机
- 网络相机
- 其他工业相机

### 2. 高级功能支持
- 相机参数自动配置
- 多相机同步
- 相机状态监控

### 3. 性能优化
- 相机缓存机制
- 并行处理支持
- 内存管理优化 