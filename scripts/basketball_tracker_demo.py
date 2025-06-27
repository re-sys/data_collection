#!/usr/bin/env python3
# -- coding: utf-8 --

import numpy as np
import cv2
import argparse

class BasketballTracker:
    def __init__(self):
        # 初始化背景差分器 (MOG2)
        self.bg_subtractor = cv2.createBackgroundSubtractorMOG2(
            history=500,        # 历史帧数
            varThreshold=25.0,  # 方差阈值
            detectShadows=False # 不检测阴影
        )
        
        # HSV阈值参数 (橙色篮球)
        self.hsv_low = np.array([2, 149, 22])
        self.hsv_high = np.array([2, 217, 230])
        
        # 形态学操作参数
        self.kernel_size = 17
        self.morph_iterations = 4
        
        # 跟踪状态
        self.tracking_initialized = False
        self.last_center = None
        self.last_radius = 0
        self.lost_frames = 0
        self.max_lost_frames = 30
        
        # 背景模型初始化状态
        self.bg_initialized = False
        self.bg_init_frames = 0
        self.bg_init_required = 30  # 需要30帧来初始化背景模型
        
    def update_background_model(self, frame):
        """更新背景模型"""
        if not self.bg_initialized:
            # 应用背景差分器来学习背景
            fg_mask = self.bg_subtractor.apply(frame, learningRate=0.01)
            self.bg_init_frames += 1
            
            if self.bg_init_frames >= self.bg_init_required:
                self.bg_initialized = True
                print("背景模型初始化完成!")
        else:
            # 正常更新背景模型
            fg_mask = self.bg_subtractor.apply(frame, learningRate=0.01)
            
        return fg_mask
    
    def detect_basketball(self, frame, motion_mask):
        """检测篮球"""
        # 1. 转换到HSV色彩空间
        hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
        
        # 2. HSV阈值分割
        hsv_mask = cv2.inRange(hsv, self.hsv_low, self.hsv_high)
        
        # 3. 结合运动检测和颜色检测
        combined_mask = cv2.bitwise_and(hsv_mask, motion_mask)
        
        # 4. 形态学操作
        kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (self.kernel_size, self.kernel_size))
        combined_mask = cv2.morphologyEx(combined_mask, cv2.MORPH_CLOSE, kernel, iterations=self.morph_iterations)
        
        # 5. 中值滤波去噪
        combined_mask = cv2.medianBlur(combined_mask, 17)
        
        # 6. 查找轮廓
        contours, _ = cv2.findContours(combined_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        if contours:
            # 找到最大的轮廓
            largest_contour = max(contours, key=cv2.contourArea)
            area = cv2.contourArea(largest_contour)
            
            # 面积阈值过滤
            if area > 500:  # 最小面积阈值
                # 拟合最小外接圆
                (x, y), radius = cv2.minEnclosingCircle(largest_contour)
                center = (int(x), int(y))
                radius = int(radius)
                
                # 简单的跟踪逻辑
                if self.tracking_initialized and self.last_center is not None:
                    # 计算距离
                    distance = np.sqrt((center[0] - self.last_center[0])**2 + 
                                     (center[1] - self.last_center[1])**2)
                    
                    # 如果距离合理，更新跟踪
                    if distance < 100:  # 最大跳变距离
                        self.last_center = center
                        self.last_radius = radius
                        self.lost_frames = 0
                    else:
                        self.lost_frames += 1
                else:
                    # 首次检测
                    self.last_center = center
                    self.last_radius = radius
                    self.tracking_initialized = True
                    self.lost_frames = 0
                
                return True, center, radius, combined_mask
        
        # 没有检测到篮球
        self.lost_frames += 1
        if self.lost_frames > self.max_lost_frames:
            self.tracking_initialized = False
            self.last_center = None
            
        return False, None, 0, combined_mask
    
    def reset_background(self):
        """重置背景模型"""
        self.bg_subtractor = cv2.createBackgroundSubtractorMOG2(
            history=500, varThreshold=25.0, detectShadows=False
        )
        self.bg_initialized = False
        self.bg_init_frames = 0
        print("背景模型已重置")

def main():
    parser = argparse.ArgumentParser(description='Basketball Tracker with Background Subtraction')
    parser.add_argument('--source', type=str, default='0', 
                       help='Video source (0 for webcam, or path to video file)')
    parser.add_argument('--output', type=str, default='', 
                       help='Output video file (optional)')
    args = parser.parse_args()
    
    # 创建篮球追踪器
    tracker = BasketballTracker()
    
    # 打开视频源
    if args.source.isdigit():
        cap = cv2.VideoCapture(int(args.source))
    else:
        cap = cv2.VideoCapture(args.source)
    
    if not cap.isOpened():
        print(f"Error: Could not open video source {args.source}")
        return
    
    # 获取视频属性
    fps = int(cap.get(cv2.CAP_PROP_FPS))
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    
    # 设置输出视频
    out = None
    if args.output:
        fourcc = cv2.VideoWriter_fourcc(*'mp4v')
        out = cv2.VideoWriter(args.output, fourcc, fps, (width, height))
    
    print("Basketball Tracker with Background Subtraction")
    print("Press 'q' to exit, 'r' to reset background model")
    print("Press 'h' to show/hide HSV mask, 'm' to show/hide motion mask")
    
    show_hsv = False
    show_motion = False
    
    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                print("End of video or failed to read frame")
                break
            
            # 更新背景模型并获取运动掩码
            motion_mask = tracker.update_background_model(frame)
            
            # 检测篮球
            detected, center, radius, final_mask = tracker.detect_basketball(frame, motion_mask)
            
            # 创建显示图像
            display_frame = frame.copy()
            
            # 绘制检测结果
            if detected and center is not None:
                # 绘制圆形
                cv2.circle(display_frame, center, radius, (0, 255, 0), 2)
                cv2.circle(display_frame, center, 2, (0, 0, 255), -1)
                
                # 显示坐标和半径
                text = f"Center: ({center[0]}, {center[1]}) Radius: {radius}"
                cv2.putText(display_frame, text, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 
                           0.7, (0, 255, 0), 2)
            
            # 显示状态信息
            status = "Tracking" if detected else "Searching"
            cv2.putText(display_frame, f"Status: {status}", (10, 60), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
            
            if not tracker.bg_initialized:
                cv2.putText(display_frame, f"Initializing BG: {tracker.bg_init_frames}/{tracker.bg_init_required}", 
                           (10, 90), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2)
            
            # 显示原始图像
            cv2.imshow('Basketball Tracker', display_frame)
            
            # 显示运动掩码
            if show_motion:
                cv2.imshow('Motion Mask', motion_mask)
            else:
                cv2.destroyWindow('Motion Mask')
            
            # 显示HSV掩码
            if show_hsv:
                hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
                hsv_mask = cv2.inRange(hsv, tracker.hsv_low, tracker.hsv_high)
                cv2.imshow('HSV Mask', hsv_mask)
            else:
                cv2.destroyWindow('HSV Mask')
            
            # 显示最终检测掩码
            cv2.imshow('Final Detection Mask', final_mask)
            
            # 保存输出视频
            if out:
                out.write(display_frame)
            
            # 键盘控制
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break
            elif key == ord('r'):
                tracker.reset_background()
            elif key == ord('h'):
                show_hsv = not show_hsv
            elif key == ord('m'):
                show_motion = not show_motion
                
    except KeyboardInterrupt:
        print("Interrupted by user")
    finally:
        # 清理资源
        cap.release()
        if out:
            out.release()
        cv2.destroyAllWindows()
        print("Video processing completed")

if __name__ == "__main__":
    main() 