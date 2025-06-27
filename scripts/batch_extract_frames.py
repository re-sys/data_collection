#!/usr/bin/env python3
"""
batch_extract_frames.py  —  批量将视频拆分为图片

用法：
    python3 batch_extract_frames.py [--stride 1] [--start 0] [--end -1]

参数说明：
    --stride  每隔多少帧保存一张（默认1，表示全部帧）
    --start   从第几帧开始截取（含，默认0）
    --end     截取到第几帧（含，默认-1 表示直到视频结束）

会自动处理 hoop_videos/ 中的所有视频文件，输出到 hoop_frames/ 对应的子文件夹中
"""

import os
import argparse
import subprocess
from pathlib import Path


def extract_video_frames(video_path: str, output_dir: str, stride: int = 1, start: int = 0, end: int = -1):
    """使用extract_frames.py脚本处理单个视频"""
    cmd = [
        "python3", "scripts/extract_frames.py",
        "--video", video_path,
        "--out", output_dir,
        "--stride", str(stride),
        "--start", str(start),
        "--end", str(end)
    ]
    
    print(f"处理视频: {video_path}")
    print(f"输出目录: {output_dir}")
    print(f"命令: {' '.join(cmd)}")
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    if result.returncode == 0:
        print(f"✓ 成功处理: {video_path}")
        print(result.stdout)
    else:
        print(f"✗ 处理失败: {video_path}")
        print(f"错误信息: {result.stderr}")
    
    print("-" * 50)


def main():
    parser = argparse.ArgumentParser(description="批量将视频拆分为图片帧")
    parser.add_argument("--stride", type=int, default=1, help="帧间隔，默认1")
    parser.add_argument("--start", type=int, default=0, help="起始帧，默认0")
    parser.add_argument("--end", type=int, default=-1, help="结束帧，默认-1(到最后)")
    args = parser.parse_args()

    # 检查输入目录
    videos_dir = Path("hoop_videos")
    if not videos_dir.exists():
        print(f"错误: 视频目录 {videos_dir} 不存在")
        return

    # 创建输出目录
    frames_dir = Path("hoop_frames")
    frames_dir.mkdir(exist_ok=True)

    # 支持的视频格式
    video_extensions = {'.mp4', '.avi', '.mov', '.mkv', '.flv', '.wmv'}

    # 查找所有视频文件
    video_files = []
    for file_path in videos_dir.iterdir():
        if file_path.is_file() and file_path.suffix.lower() in video_extensions:
            video_files.append(file_path)

    if not video_files:
        print(f"在 {videos_dir} 中没有找到视频文件")
        return

    print(f"找到 {len(video_files)} 个视频文件:")
    for video_file in video_files:
        print(f"  - {video_file.name}")

    print(f"\n开始批量处理...")
    print("=" * 50)

    # 处理每个视频文件
    for video_file in video_files:
        # 创建对应的输出目录（使用视频文件名，不含扩展名）
        output_subdir = frames_dir / video_file.stem
        output_subdir.mkdir(exist_ok=True)
        
        # 提取帧
        extract_video_frames(
            str(video_file),
            str(output_subdir),
            args.stride,
            args.start,
            args.end
        )

    print("批量处理完成！")
    print(f"所有帧已保存到 {frames_dir} 目录中")


if __name__ == "__main__":
    main() 