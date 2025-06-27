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

        # 设置曝光时间为3000
        ret = self.cam.MV_CC_SetFloatValue("ExposureTime", 3000.0)
        if ret != 0:
            print("Set exposure time fail!")
            return False

        # 设置手动增益模式
        ret = self.cam.MV_CC_SetEnumValue("GainAuto", 0)  # 0代表手动增益模式
        if ret != 0:
            print("Set manual gain mode fail!")
            return False

        # 设置增益值为29
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

def main():
    # 创建相机实例
    camera = HikCamera()
    
    # 打开相机
    if not camera.open_camera():
        print("Failed to open camera!")
        return
        
    print("Camera opened successfully!")
    print("Press 'q' to exit")
    
    try:
        while True:
            # 获取图像
            ret, frame = camera.get_frame()
            if ret:
                # 显示图像
                cv2.imshow('Hikvision Camera', frame)
                
                # 按q退出
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
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