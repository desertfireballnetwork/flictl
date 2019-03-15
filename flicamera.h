#pragma once

#include "libflipro.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <cmath>

#define FLICAMERA_MAX_SUPPORTED_CAMERAS (4)

#define FLICAMERA_FRAME_SIZE_ADD_BULGARIAN_CONSTATNT (10)

#define FLICAMERA_GSENSE4040_SENSOR_WIDTH (4096)
#define FLICAMERA_GSENSE4040_SENSOR_HEIGHT (4096)

class FliCameraC
{
 private:
  int32_t siDeviceHandle;  
  uint8_t *pFrame;    // allocted in allocFrameFullRes()
  uint32_t uiCamCapSize;
  bool isDeviceOpen;
  uint32_t uiFrameSizeInBytes;
  FPROGAINVALUE *pNewTableLow, *pNewTableHigh;
  bool isExternalTriggerEnabled;
  FPROEXTTRIGTYPE externalTriggerType;
  
 public:
  uint32_t uiNumDetectedDevices;
  FPRODEVICEINFO s_camDeviceInfo[FLICAMERA_MAX_SUPPORTED_CAMERAS];
  FPROCAP        s_camCapabilities;
  bool weKnowCapabilities;
  int32_t sensorTemp;
  double ambientTemp;
  double baseTemp;
  double coolerTemp;
  uint32_t uiPixelDepth;
  uint32_t uiPixelLSB;
  uint64_t exposureTime;
  uint64_t frameDelay;
  
  FliCameraC();
  ~FliCameraC();
  
  bool listDevices( int32_t maxNumDevices );
  bool listDevices();
  bool openDevice();
  bool closeDevice();
  bool getCapabilities();
  bool getPixelConfig();
  void printCapabilities();

  bool getPrintConfig();
    
  bool setShutterUserControl(bool state);
  bool getShutterUserControl();
  bool setShutter(bool state);

  bool getExternalTriggerEnable();
  bool getExternalTriggerEnable(bool* bEnable, FPROEXTTRIGTYPE* pTrigType);
  bool setExternalTriggerEnable(bool bEnable, FPROEXTTRIGTYPE pTrigType); 
  
  bool getAllTemperatures();
  void printAllTemperatures();

  bool printModes();
  bool setMode(uint32_t mode);
  
  bool setTemperatureSetPoint(double dblSetPoint);

  // Note:FPROSensor_SetHDREnable() is missing in libflipro, we can only read
  // the state with FPROSensor_GetHDREnable(int32_t iHandle, bool *pEnable)
  // bool setHDRMode(bool state);

  bool setExpTime(uint64_t exposureTime);
  bool setExpDelay(uint64_t exposureDelay);

  bool setLowGain(uint32_t gainIndex);
  bool setHighGain(uint32_t gainIndex);
 
  bool allocFrameFullRes();
  bool prepareCaptureFullSensor();
  bool grabImage();
  //  bool grabImages(uint32_t num);

  bool startCapture(uint32_t num);
  bool getImage();
  bool stopCapture();
  bool endCapture();
  // bool abortCapture();

  bool extractMetaData( uint8_t* pMetaData, uint32_t *metaDataSize );
  bool convertHdrRawToBitmaps16bit( uint16_t* bitamp16bitLow, uint16_t* bitamp16bitHigh );
  bool convertLdrRawToBitmap16bit( uint16_t* bitamp16bit );

  void* getImagePtr();
};
