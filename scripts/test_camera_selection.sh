#!/bin/bash

# 测试相机选择功能
echo "=== 测试相机选择功能 ==="

# 检查可执行文件是否存在
if [ ! -f "../build/trajectory_solver" ]; then
    echo "错误: 找不到 trajectory_solver 可执行文件"
    echo "请先编译项目: cd build && make"
    exit 1
fi

# 检查配置文件是否存在
if [ ! -f "../config/example_config.yaml" ]; then
    echo "错误: 找不到配置文件 config/example_config.yaml"
    exit 1
fi

echo "1. 测试帮助信息..."
../build/trajectory_solver

echo ""
echo "2. 测试无效模式..."
../build/trajectory_solver invalid_mode

echo ""
echo "3. 测试相机类型列表..."
echo "支持的相机类型:"
echo "  - hikvision (海康相机)"
echo "  - zed (ZED相机)"

echo ""
echo "4. 测试配置文件加载..."
../build/trajectory_solver camera ../config/example_config.yaml

echo ""
echo "=== 测试完成 ==="
echo ""
echo "使用方法:"
echo "  # 使用配置文件中的相机类型"
echo "  ../build/trajectory_solver camera ../config/example_config.yaml"
echo ""
echo "  # 强制使用海康相机"
echo "  ../build/trajectory_solver camera_hik ../config/example_config.yaml"
echo ""
echo "  # 强制使用ZED相机"
echo "  ../build/trajectory_solver camera_zed ../config/example_config.yaml" 