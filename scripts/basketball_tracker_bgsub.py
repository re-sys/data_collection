#!/usr/bin/env python3
# -- coding: utf-8 --

import sys
import numpy as np
import cv2
from ctypes import *

sys.path.append("/opt/MVS/Samples/64/Python/MvImport")
from MvCameraControl_class import *

class HikCamera:
    def __init__(self):
        self.cam = MvCamera()
        
    def open_camera(self):
        # 初始化SDK
        ret = MvCamera.MV_CC_Initialize()
        if ret != 0:
            print("Initialize SDK fail!")
            return False
            
        # 枚举设备
        deviceList = MV_CC_DEVICE_INFO_LIST()
        tlayerType = MV_GIGE_DEVICE | MV_USB_DEVICE
        ret = MvCamera.MV_CC_EnumDevices(tlayerType, deviceList)
        if ret != 0:
            print("Enum devices fail!")
            return False
            
        if deviceList.nDeviceNum == 0:
            print("No camera found!")
            return False
            
        # 默认选择第一个设备
        stDeviceList = cast(deviceList.pDeviceInfo[0], POINTER(MV_CC_DEVICE_INFO)).contents
        
        # 创建相机
        ret = self.cam.MV_CC_CreateHandle(stDeviceList)
        if ret != 0:
            print("Create handle fail!")
            return False
            
        # 打开设备
        ret = self.cam.MV_CC_OpenDevice(MV_ACCESS_Exclusive, 0)
        if ret != 0:
            print("Open device fail!")
            return False

        # 设置手动曝光模式
        ret = self.cam.MV_CC_SetEnumValue("ExposureAuto", 0)  # 0代表手动曝光模式
        if ret != 0:
            print("Set manual exposure mode fail!")
            return False

        # 设置曝光时间为20000
        ret = self.cam.MV_CC_SetFloatValue("ExposureTime", 20000.0)
        if ret != 0:
            print("Set exposure time fail!")
            return False

        # 设置手动增益模式
        ret = self.cam.MV_CC_SetEnumValue("GainAuto", 0)  # 0代表手动增益模式
        if ret != 0:
            print("Set manual gain mode fail!")
            return False

        # 设置增益值为23.9
        ret = self.cam.MV_CC_SetFloatValue("Gain", 23.9)
        if ret != 0:
            print("Set gain value fail!")
            return False

        # 设置为RGB8像素格式
        ret = self.cam.MV_CC_SetEnumValue("PixelFormat", PixelType_Gvsp_RGB8_Packed)
        if ret != 0:
            print("Set pixel format fail!")
            return False
            
        # 设置触发模式为off
        ret = self.cam.MV_CC_SetEnumValue("TriggerMode", MV_TRIGGER_MODE_OFF)
        if ret != 0:
            print("Set trigger mode fail!")
            return False
            
        # 开始取流
        ret = self.cam.MV_CC_StartGrabbing()
        if ret != 0:
            print("Start grabbing fail!")
            return False
            
        return True
        
    def get_frame(self):
        stOutFrame = MV_FRAME_OUT()
        memset(byref(stOutFrame), 0, sizeof(stOutFrame))
        
        ret = self.cam.MV_CC_GetImageBuffer(stOutFrame, 1000)
        if ret == 0:
            # 将图像数据转换为numpy数组
            data = (c_ubyte * stOutFrame.stFrameInfo.nFrameLen)()
            memmove(data, stOutFrame.pBufAddr, stOutFrame.stFrameInfo.nFrameLen)
            frame = np.frombuffer(data, dtype=np.uint8)
            
            # 重塑数组为RGB格式
            frame = frame.reshape((stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nWidth, 3))
            
            # 确保是BGR格式（OpenCV默认格式）
            frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
                
            self.cam.MV_CC_FreeImageBuffer(stOutFrame)
            return True, frame
        else:
            return False, None
            
    def close_camera(self):
        # 停止取流
        self.cam.MV_CC_StopGrabbing()
        # 关闭设备
        self.cam.MV_CC_CloseDevice()
        # 销毁句柄
        self.cam.MV_CC_DestroyHandle()
        # 反初始化SDK
        MvCamera.MV_CC_Finalize()

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
    # 创建相机实例
    camera = HikCamera()
    
    # 创建篮球追踪器
    tracker = BasketballTracker()
    
    # 打开相机
    if not camera.open_camera():
        print("Failed to open camera!")
        return
        
    print("Camera opened successfully!")
    print("Press 'q' to exit, 'r' to reset background model")
    
    try:
        while True:
            # 获取图像
            ret, frame = camera.get_frame()
            if ret:
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
                cv2.imshow('Motion Mask', motion_mask)
                
                # 显示最终检测掩码
                cv2.imshow('Final Detection Mask', final_mask)
                
                # 键盘控制
                key = cv2.waitKey(1) & 0xFF
                if key == ord('q'):
                    break
                elif key == ord('r'):
                    tracker.reset_background()
                    
            else:
                print("Failed to get frame!")
                
    except KeyboardInterrupt:
        print("Interrupted by user")
    finally:
        # 清理资源
        cv2.destroyAllWindows()
        camera.close_camera()
        print("Camera closed")

if __name__ == "__main__":
    main() 