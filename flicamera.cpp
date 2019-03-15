#include "flicamera.h"

//--------------------------------------------------------------

FliCameraC::FliCameraC():
  uiNumDetectedDevices(FLICAMERA_MAX_SUPPORTED_CAMERAS)
{
  pFrame = NULL;
  pNewTableLow = NULL;
  pNewTableHigh = NULL;
  uiCamCapSize = sizeof(FPROCAP);
  weKnowCapabilities = false;
  sensorTemp = 0;
  ambientTemp = 0;
  baseTemp = 0;
  coolerTemp = 0;
  isDeviceOpen = false;
  isExternalTriggerEnabled = false;
}

//--------------------------------------------------------------

FliCameraC::~FliCameraC()
{
  std::cout << "FliCameraC::destructor DEBUG: ";
  std::cout.flush();
  if( pFrame != NULL )
    {
      std::cout << "freeing pFrame... ";
      std::cout.flush();
      free( pFrame );
      std::cout << "OK" << std::endl;
    }
  else
    {
      std::cout << "NOT freeing pFrame." << std::endl;
    }
  this->closeDevice();
}

//--------------------------------------------------------------

bool FliCameraC::listDevices( int32_t maxNumDevices )
{
  uiNumDetectedDevices = maxNumDevices;
  return FliCameraC::listDevices();
}

//--------------------------------------------------------------
/// return 1 if succeeded, 0 if failed (
bool FliCameraC::listDevices()
{
  int32_t iResult = -1;
  iResult = FPROCam_GetCameraList(&(s_camDeviceInfo[0]), &uiNumDetectedDevices);
  return ((iResult >= 0) && (uiNumDetectedDevices > 0));
}

//--------------------------------------------------------------
/// open 1st device on the list
/// return 1 if succeeded, 0 if failed 
bool FliCameraC::openDevice()
{
  int32_t iResult = -1;
  siDeviceHandle = -1;
  iResult = FPROCam_Open(&(s_camDeviceInfo[0]), &siDeviceHandle);
  isDeviceOpen = ((iResult >= 0) && (siDeviceHandle >= 0));
  return isDeviceOpen;
}

//--------------------------------------------------------------
/// open 1st device on the list
/// return 1 if succeeded, 0 if failed 
bool FliCameraC::closeDevice()
{
  int32_t iResult = -1;
  iResult = FPROCam_Close( siDeviceHandle );
  if(iResult >= 0)
    {
      isDeviceOpen = false;
      siDeviceHandle = -1;      
      return true;
    }
  else
    {
      return false;
    }
}

//--------------------------------------------------------------
/// return 1 if succeeded, 0 if failed 
bool FliCameraC::getCapabilities()
{
  int32_t iResult = -1;
  weKnowCapabilities = false;
  iResult = FPROSensor_GetCapabilities(siDeviceHandle, &s_camCapabilities, &uiCamCapSize);
  weKnowCapabilities = (iResult >=0);
  return weKnowCapabilities;
}

//--------------------------------------------------------------
/// return 1 if succeeded, 0 if failed 
bool FliCameraC::getPixelConfig()
{
  int32_t iResult = -1;
  iResult = FPROFrame_GetPixelConfig(siDeviceHandle, &uiPixelDepth, &uiPixelLSB);
  return (iResult >=0);
}

//--------------------------------------------------------------
/// return 1 if succeeded, 0 if failed 
void FliCameraC::printCapabilities()
{
  uint32_t uiGainEntries;
  uint32_t uiGainIndex;
  float    fGainValue;

  if( !weKnowCapabilities )
    {
      getCapabilities();
    }
  if( weKnowCapabilities )
    {
      // Print capabilities of device.
      printf("Device capabilities: s_camCapabilities\n");
      printf("  uiSize: %d\n", s_camCapabilities.uiSize);				
      printf("  uiCapVersion: %d\n", s_camCapabilities.uiCapVersion);
      printf("  uiDeviceType: %10x ", s_camCapabilities.uiDeviceType);
      switch( s_camCapabilities.uiDeviceType )
	{
	case FPRO_CAM_DEVICE_TYPE_GSENSE400:
	  printf("(Gsense 400)\n");
	  break;
	case FPRO_CAM_DEVICE_TYPE_GSENSE2020:
	  printf("(Gsense 2020)\n");
	  break;
	case FPRO_CAM_DEVICE_TYPE_GSENSE4040:
	  printf("(Gsense 4040)\n");
	  break;
	case FPRO_CAM_DEVICE_TYPE_KODAK47051:
	  printf("(Kodak 47051)\n");
	  break;
	case FPRO_CAM_DEVICE_TYPE_KODAK29050:
	  printf("(Kodak 29050)\n");
	  break;
	case FPRO_CAM_DEVICE_TYPE_DC230_42:
	  printf("(DC 230-42)\n");
	  break;
	case FPRO_CAM_DEVICE_TYPE_DC230_84:
	  printf("(DC 230-84)\n");
	  break;
	default:
	  printf("(INVALID - UNDEFINED TYPE!)\n");	  
	}
      printf("  uiMaxPixelImageWidth: %d\n", s_camCapabilities.uiMaxPixelImageWidth);
      printf("  uiMaxPixelImageHeight: %d\n", s_camCapabilities.uiMaxPixelImageHeight);
      printf("  uiAvailablePixelDepths: %d\n", s_camCapabilities.uiAvailablePixelDepths);
      printf("  uiBinningsTableSize: %d\n", s_camCapabilities.uiBinningsTableSize);
      printf("  uiBlackLevelMax: %d\n", s_camCapabilities.uiBlackLevelMax);
      printf("  uiBlackSunMax: %d\n", s_camCapabilities.uiBlackSunMax);
      printf("  uiLowGain (number of entries in the low gain table): %d\n", s_camCapabilities.uiLowGain);
      printf("  uiHighGain (number of entries in the high gain table): %d\n", s_camCapabilities.uiHighGain);
      printf("  uiReserved: %d\n", s_camCapabilities.uiReserved);
      printf("  uiRowScanTime: %d\n", s_camCapabilities.uiRowScanTime);
      printf("  uiDummyPixelNum: %d\n", s_camCapabilities.uiDummyPixelNum);
      printf("  bHorizontalScanInvertable: %d\n", s_camCapabilities.bHorizontalScanInvertable);
      printf("  bVerticalScanInvertable: %d\n", s_camCapabilities.bVerticalScanInvertable);
      printf("  uiNVStorageAvailable: %d\n", s_camCapabilities.uiNVStorageAvailable);
      printf("  uiPreFrameReferenceRows: %d\n", s_camCapabilities.uiPreFrameReferenceRows);
      printf("  uiPostFrameReferenceRows: %d\n", s_camCapabilities.uiPostFrameReferenceRows);
      printf("  uiMetaDataSize: %d\n", s_camCapabilities.uiMetaDataSize);

      printf("Gain tables:\n");

      if (s_camCapabilities.uiLowGain > 0)
	{
	  printf ("  Low gain channel gain table size: %d\n",s_camCapabilities.uiLowGain );
	  // First make sure you have allocated enough memory for the gain table you
	  // would like to retrieve- here we are getting the LDR Table.  Each entry is
	  // a uint32_t value so we allocate an array of uint32_t
	  uiGainEntries = s_camCapabilities.uiLowGain;
	  pNewTableLow = new FPROGAINVALUE[s_camCapabilities.uiLowGain];
	  if (FPROSensor_GetGainTable(siDeviceHandle, FPRO_GAIN_TABLE_LOW_CHANNEL, pNewTableLow, &uiGainEntries) >= 0)
	    {
	      printf("  Low gain channel gain table: [tableIndex, deviceIndex, gain]\n");
	      // now that you have the table entries you can process them
	      for (uint32_t i = 0; i < uiGainEntries; ++i)
		{
		  // Each gain value is scaled by the camera to produce an integer.  To return the value
		  // to a floating point representation, apply the scale factor
		  fGainValue = (float)pNewTableLow[i].uiValue / (float)FPRO_GAIN_SCALE_FACTOR;
		  // Here you could populate a drop down list for a GUI with the Gain value.
		  // Be aware that the gain values are set by there index in the table (pNewTable[i].uiDeviceIndex)
		  // just retrieved.  So you could populate a GUI drop down list, but just make sure to
		  // maintin the map between your list and this list you just received so you can prpoerly
                            // set gain indeces using FPROSensor_SetGainIndex().
		  printf ("    %d: %d = %5.3f\n", i, pNewTableLow[i].uiDeviceIndex , fGainValue );
		}
	    }
	  printf ("  High gain channel gain table size: %d\n",s_camCapabilities.uiHighGain );
	  uiGainEntries = s_camCapabilities.uiHighGain;
	  pNewTableHigh = new FPROGAINVALUE[s_camCapabilities.uiHighGain];
	  if (FPROSensor_GetGainTable(siDeviceHandle, FPRO_GAIN_TABLE_HIGH_CHANNEL, pNewTableHigh, &uiGainEntries) >= 0)
	    {
	      printf("  High gain channel gain table: [tableIndex, deviceIndex, gain]\n");
	      // now that you have the table entries you can process them
	      for (uint32_t i = 0; i < uiGainEntries; ++i)
		{
		  // Each gain value is scaled by the camera to produce an integer.  To return the value
		  // to a floating point representation, apply the scale factor
		  fGainValue = (float)pNewTableHigh[i].uiValue / (float)FPRO_GAIN_SCALE_FACTOR;
		  // Here you could populate a drop down list for a GUI with the Gain value.
		  // Be aware that the gain values are set by there index in the table (pNewTable[i].uiDeviceIndex)
		  // just retrieved.  So you could populate a GUI drop down list, but just make sure to
		  // maintin the map between your list and this list you just received so you can prpoerly
		  // set gain indeces using FPROSensor_SetGainIndex().
		  printf ("    %d: %d = %5.3f\n", i, pNewTableHigh[i].uiDeviceIndex , fGainValue );
		}
	    }
	  // Set the Low Gain to the second value in the list (index value of 1)
	  uint32_t lowIndex=1;
	  if (s_camCapabilities.uiLowGain >= (lowIndex + 1))
	    {
	      // Set the gain index for the table
	      FPROSensor_SetGainIndex(siDeviceHandle, FPRO_GAIN_TABLE_LOW_CHANNEL, pNewTableLow[lowIndex].uiDeviceIndex);
	      // Read it back- should be the same as what you set..
	      // This is not required for proper operation, just being shown
	      // for example purposes
	      FPROSensor_GetGainIndex(siDeviceHandle, FPRO_GAIN_TABLE_LOW_CHANNEL, &uiGainIndex);
	      fGainValue = (float)pNewTableLow[lowIndex].uiValue / (float)FPRO_GAIN_SCALE_FACTOR;
	      printf( "Current low gain index setting is %d, which corresponds to device index %d and gain %5.3f\n",
		      lowIndex, uiGainIndex, fGainValue );
	    }
	  // Set the High Gain to the 11th value in the list (index value of 10)
	  uint32_t highIndex=10;
	  if (s_camCapabilities.uiHighGain >= (highIndex+1))
	    {
	      // Set the gain index for the table
	      FPROSensor_SetGainIndex(siDeviceHandle, FPRO_GAIN_TABLE_HIGH_CHANNEL, pNewTableHigh[highIndex].uiDeviceIndex);
	      // Read it back- should be the same as what you set..
	      // This is not required for proper operation, just being shown
	      // for example purposes
	      FPROSensor_GetGainIndex(siDeviceHandle, FPRO_GAIN_TABLE_HIGH_CHANNEL, &uiGainIndex);
	      fGainValue = (float)pNewTableHigh[highIndex].uiValue / (float)FPRO_GAIN_SCALE_FACTOR;
	      printf( "Current high gain index setting is %d, which corresponds to device index %d and gain %5.3f\n",
		      highIndex, uiGainIndex, fGainValue );
	    }
	}
      else
	{
	  printf ("uiLowGain = %d (<=0), gain table(s) not available.\n",s_camCapabilities.uiLowGain );
	}      
    }
  else
    {
      std::cerr << "FliCameraC::printCapabilities(): we do not know capabilities." << std::endl;
    }
  printModes();
}

//--------------------------------------------------------------
/// return true if succeeded, false otherwise
bool FliCameraC::setShutterUserControl(bool state) 
{
  int32_t iResult = -1;
  iResult = FPROCtrl_SetShutterOverride(siDeviceHandle, state);
  if( iResult < 0 )
    {
      std::cerr << "FliCameraC::setShutterUserControl(): failed to set shutter user control state,"
		<< "\tFPROCtrl_SetShutterOverride retval()=" << iResult << std::endl;
    }
  return (iResult >=0);
}

//--------------------------------------------------------------
/// return true if user can control shutter, false otherwise
bool FliCameraC::getShutterUserControl() 
{
  bool state = false;
  int32_t iResult = -1;
  iResult = FPROCtrl_GetShutterOverride(siDeviceHandle, &state);
  if( iResult < 0 )
    {
      std::cerr << "FliCameraC::getShutterUserControl(): failed to get shutter user control state.\n"
		<< "\tFPROCtrl_GetShutterOverride() retval=" << iResult << std::endl;
    }
  //DEBUG std::cout << "getShutterUserControl() : Shutter user control state is " << state << std::endl; 
  return ((iResult >=0) && state);
}

//--------------------------------------------------------------
/// return true if succeeded, false if failed
bool FliCameraC::setShutter(bool state)
{
  int32_t iResult = -1;
  bool isShutterUserControl = getShutterUserControl();
  if( isShutterUserControl )
    {
      iResult = FPROCtrl_SetShutterOpen(siDeviceHandle, state);
      if( iResult < 0 )
	{
	  std::cerr << "FliCameraC::setShutter(): FPROCtrl_SetShutterOpen() failed, retval=" << iResult<< std::endl;
	}
    }
  else
    {
      std::cerr << "FliCameraC::setShutter(): shutter not in user control mode." << std::endl;
    }
  return((iResult >=0) && isShutterUserControl);
}

//--------------------------------------------------------------
/// return true if user can control shutter, false otherwise
bool FliCameraC::getExternalTriggerEnable()
{
  int32_t iResult = -1;
  iResult = FPROCtrl_GetExternalTriggerEnable(siDeviceHandle, &isExternalTriggerEnabled, &externalTriggerType);
  if( iResult < 0 )
    {
      std::cerr << "FliCameraC::getExternalTriggerEnable(): FPROCtrl_GetExternalTriggerEnable() failed, retval=" << iResult<< std::endl;
      return false;
    }
  else
    {
      return true;
    }
}

//--------------------------------------------------------------
/// return true if user can control shutter, false otherwise
bool FliCameraC::getExternalTriggerEnable(bool* bEnable, FPROEXTTRIGTYPE* pTrigType)
{
  if( getExternalTriggerEnable() )
    {
      *bEnable = isExternalTriggerEnabled;
      *pTrigType = externalTriggerType;
      return true;
    }
  else
    {
      return false;
    }
}

//--------------------------------------------------------------
/// return true if user can control shutter, false otherwise
bool FliCameraC::setExternalTriggerEnable(bool bEnable, FPROEXTTRIGTYPE pTrigType) 
{
  int32_t iResult = -1;

  iResult = FPROCtrl_SetExternalTriggerEnable(siDeviceHandle, bEnable, pTrigType);
  if( iResult < 0 )
    {
      std::cerr << "FliCameraC::getExternalTriggerEnable(): FPROCtrl_SetExternalTriggerEnable() failed, retval=" << iResult<< std::endl;
      return false;
    }
  else
    {
      isExternalTriggerEnabled = bEnable;
      externalTriggerType = pTrigType;
      return true;
    }  
}

//--------------------------------------------------------------
/// return true if succeeded, false if failed
bool FliCameraC::getAllTemperatures()
{
  int32_t iResult = -1;
  // sensor temp read fails on later build FLI Kepler 4040 cameras! (error -2)
  //iResult = FPROCtrl_GetSensorTemperature(siDeviceHandle, &sensorTemp);
  //if( iResult >=0 )
  //  {
  iResult = FPROCtrl_GetTemperatures(siDeviceHandle, &ambientTemp, &baseTemp, &coolerTemp );
  if( iResult < 0 )
    {
      std::cerr << "FliCameraC::getAllTemperatures(): failed to read aux temepartures.\n"
		<< "\tFPROCtrl_GetTemperatures() retval=" << iResult << std::endl;
    }
  //  }
  //else
  //  {
  //    std::cerr << "FliCameraC::getAllTemperatures(): failed to read sensor temeparture.\n"
  //		<< "\tFPROCtrl_GetSensorTemperature() retval=" << iResult << std::endl;
  //  }
  
  return( iResult >=0 );
}

//--------------------------------------------------------------

void FliCameraC::printAllTemperatures()
{
  printf("Temperatures (C)\n");
  // sensor temp is not availabla on FLI Kepler 4040 cameras!
  //  printf("  Sensor  %d\n", sensorTemp);
  printf("  Ambient %5.1f\n", ambientTemp);
  printf("  Base    %5.1f\n", baseTemp);
  printf("  Cooler  %5.1f\n", coolerTemp);
}

//--------------------------------------------------------------
/// return true if succeeded, false if failed
bool FliCameraC::printModes()
{
  int32_t      iResult;
  uint32_t     uiModeCount;
  uint32_t     uiCurrentMode;
  FPROSENSMODE modeInfo;
  uint32_t     i;

  // Get the numer of available modes and the current mode setting (index)
  iResult= FPROSensor_GetModeCount(siDeviceHandle, &uiModeCount, &uiCurrentMode);

  if( iResult >= 0 )
    {
      printf( "List of avaliable camera modes (number of modes: %d)\n", uiModeCount);
      // get each of the modes- and process
      for( i = 0; (i < uiModeCount) && (iResult >= 0); ++i )
	{
	  iResult = FPROSensor_GetMode(siDeviceHandle, i, &modeInfo);
	  if( iResult < 0 )
	    {
	      std::cerr << "FliCameraC::printModes() ERROR: FPROSensor_GetMode() failed." << std::endl;
	      continue;
	    }
	  if( iResult >= 0 )
	    {
	      // You now have the name of the mode for index 'i'
	      // For convenience, the index of the mode is also returned in the
	      // FPROSENSMODE structure.  It is this index you will use in FPROSensor_SetMode()
	      printf( "  List index: %d device mode index: %d mode name: %s\n", i, modeInfo.uiModeIndex, (char* )(modeInfo.wcModeName) );
	    }
	}
      printf( "Current camera mode: %d\n", uiCurrentMode );
    }
  else
    {
       std::cerr << "FliCameraC::printModes() ERROR: FPROSensor_GetModeCount() failed." << std::endl;
    }
    
  return (iResult >= 0);
}

//--------------------------------------------------------------
/// return true if succeeded, false if failed
//bool FliCameraC::getMode(uint31_t* mode);

//--------------------------------------------------------------
/// return true if succeeded, false if failed
bool FliCameraC::setMode(uint32_t mode)
{
  int32_t      iResult;
  uint32_t     uiModeCount;
  uint32_t     uiCurrentMode;

  // Get the numer of available modes and the current mode setting (index)
  iResult= FPROSensor_GetModeCount(siDeviceHandle, &uiModeCount, &uiCurrentMode);
  if( iResult >= 0 )
    {
      // Set the current mode 
      if( mode < uiModeCount )
	{
	  iResult = FPROSensor_SetMode( siDeviceHandle, mode );
	  return (iResult >= 0);
	}
      else
	{
	  std::cerr << "FliCameraC::setMode() ERROR: FPROSensor_SetMode() index %d is out of range 0..%d"
		    << mode << uiModeCount << std::endl;	  
	  return false;
	}
    }
  else
    {
       std::cerr << "FliCameraC::setMode() ERROR: FPROSensor_GetModeCount() failed." << std::endl;
    }
  return false;
}

//--------------------------------------------------------------
/// return true if succeeded, false if failed
bool FliCameraC::setTemperatureSetPoint( double dblSetPoint )
{
  int32_t iResult;
  double readBackSetPoint;
  
  iResult = FPROCtrl_SetTemperatureSetPoint(siDeviceHandle, dblSetPoint);
  if( iResult < 0 )
    {
      std::cerr << "FliCameraC::setTemperatureSetPoint() ERROR: FPROFrame_SetTemperatureSetPoint() failed, retval="
		<< iResult << std::endl;
      return false;
    }
  else
    {
      iResult = FPROCtrl_GetTemperatureSetPoint(siDeviceHandle, &readBackSetPoint);
       if( iResult < 0 )
	 {
	   std::cerr << "FliCameraC::setTemperatureSetPoint() ERROR: FPROFrame_GetTemperatureSetPoint() failed, retval="
		     << iResult << std::endl;
	   return false;
	 }
       else
	 {
	   if( readBackSetPoint == dblSetPoint )
	     {
	       return true;
	     }
	   else
	     {
	       std::cerr << "FliCameraC::setTemperatureSetPoint() ERROR: read back temperature point "
			 << std::fixed << readBackSetPoint
			 << " differs from requested value " << dblSetPoint << std::endl;	       
	       return false;
	     }
	 }
    }
}

//--------------------------------------------------------------
/// return true if succeeded, false if failed
/*
bool setHDRMode(bool state)
{
  Note: there is only FPROSensor_GetHDREnable (int32_t iHandle, bool *pEnable), 
        but not FPROSensor_SetHDREnable (int32_t iHandle, bool pEnable) inthe libflipro!
}
*/

//--------------------------------------------------------------
/// set exposure time in nanoseconds
/// return true if succeeded, false if failed
bool FliCameraC::setExpTime(uint64_t exposureTime)
{
  int32_t iResult = -1; // assuming failure
  uint64_t origExposureTime, readExposureTime, readFrameDelay;
  bool immediate = false;
  
  iResult = FPROCtrl_GetExposure(siDeviceHandle, &readExposureTime, &readFrameDelay, &immediate);
  if( iResult < 0 )
    {
      std::cerr << "FliCameraC::setExpTime() ERROR: FPROCtrl_GetExposure() failed, retval=" << iResult << std::endl;
      return false;      
    }
  else
    {
      // store real read values to class data members
      this->exposureTime = readExposureTime;
      this->frameDelay = readFrameDelay;
      iResult = FPROCtrl_SetExposure(siDeviceHandle, exposureTime, readFrameDelay, immediate);
      if( iResult < 0 )
	{
	  std::cerr << "FliCameraC::setExpTime() ERROR: FPROCtrl_SetExposure() failed, retval=" << iResult << std::endl;
	  return false;      
	}
      else
	{
	  // set exposure succeeded, let's read back the value a nd verify it
	  origExposureTime = readExposureTime;
	  iResult = FPROCtrl_GetExposure(siDeviceHandle, &readExposureTime, &readFrameDelay, &immediate);
	  if( iResult < 0 )
	    {
	      std::cerr << "FliCameraC::setExpTime() ERROR: FPROCtrl_GetExposure() failed, retval=" << iResult << std::endl;
	      return false;      
	    }
	  else
	    {
	      // store real read values to class data members
	      this->exposureTime = readExposureTime;
	      this->frameDelay = readFrameDelay;
	      // re-read of the set value succeeded, let's compare what we read with what we have set
	      if( readExposureTime == exposureTime )
		{
		  std::cout << "FliCameraC::setExpTime() DEBUG: exposure time [ns] changed "
			  << origExposureTime << " -> "<< exposureTime << std::endl;		  
		  return true;
		}
	      else
		{
		  // the read back value is often not identical, but should differ less than 1% from the requested value
		  if( (fabs((double)readExposureTime - (double)exposureTime) / (double)(exposureTime)) < 0.01 )
		    {
		      std::cout << "FliCameraC::setExpTime() WARNING: exposure time [ns] changed "
				<< origExposureTime << " -> "<< readExposureTime
				<< ", but requested " << exposureTime << std::endl;
		      return true;
		    }
		  else
		    {
		      std::cerr << "FliCameraC::setExpTime() ERROR: set exposure time " << exposureTime
			      << ", but reads back " << readExposureTime << std::endl;
		      return false;
		    }		  
		}
	    }
	}
    }
}

//--------------------------------------------------------------
/// set exposure delay in nanoseconds
/// return true if succeeded, false if failed
bool FliCameraC::setExpDelay(uint64_t exposureDelay)
{
  int32_t iResult = -1; // assuming failure
  uint64_t origFrameDelay, readExposureTime, readFrameDelay;
  bool immediate = false;
  
  iResult = FPROCtrl_GetExposure(siDeviceHandle, &readExposureTime, &readFrameDelay, &immediate);
  if( iResult < 0 )
    {
      std::cerr << "FliCameraC::setExpDelay() ERROR: FPROCtrl_GetExposure() failed, retval=" << iResult << std::endl;
      return false;      
    }
  else
    {
      // store real read values to class data members
      this->exposureTime = readExposureTime;
      this->frameDelay = readFrameDelay;
      iResult = FPROCtrl_SetExposure(siDeviceHandle, readExposureTime, exposureDelay, immediate);
      if( iResult < 0 )
	{
	  std::cerr << "FliCameraC::setExpDelay() ERROR: FPROCtrl_SetExposure() failed, retval=" << iResult << std::endl;
	  return false;      
	}
      else
	{
	  // set exposure succeeded, let's read back the value a nd verify it
	  origFrameDelay = readFrameDelay;
	  iResult = FPROCtrl_GetExposure(siDeviceHandle, &readExposureTime, &readFrameDelay, &immediate);
	  if( iResult < 0 )
	    {
	      std::cerr << "FliCameraC::setExpDelay() ERROR: FPROCtrl_GetExposure() failed, retval=" << iResult << std::endl;
	      return false;      
	    }
	  else
	    {
	      // store real read values to class data members
	      this->exposureTime = readExposureTime;
	      this->frameDelay = readFrameDelay;
	      // re-read of the set value succeeded, let's compare what we read with what we have set
	      if( readFrameDelay == exposureDelay )
		{
		  std::cout << "FliCameraC::setExpDelay() DEBUG: exposure (frame) delay [ns] changed "
			    << readFrameDelay << " -> "<< exposureDelay << std::endl;
		  return true;
		}
	      else
		{
		  // the read back value is often not identical, but should differ less than 1% from the requested value
		  if( (fabs((double)readFrameDelay - (double)exposureDelay) / (double)(exposureDelay)) < 0.01 )
		    {
		      std::cout << "FliCameraC::setExpDelay() WARNING: exposure time [ns] changed "
				<< origFrameDelay << " -> "<< readFrameDelay
				<< ", but requested " << exposureDelay << std::endl;
		      return true;
		    }
		  else
		    {
		      std::cerr << "FliCameraC::setExpDelay() ERROR: set exposure time " << exposureDelay
			      << ", but reads back " << readFrameDelay << std::endl;
		      return false;
		    }		  
		}
	    }
	}	
    }
}

//--------------------------------------------------------------
/// set exposure delay in nanoseconds
/// return true if succeeded, false if failed
bool FliCameraC::setLowGain(uint32_t gainIndex)
{
  uint32_t uiGainDeviceIndex;
  uint32_t uiNumGainEntries;
  float    fGainValue;
  
  if( !weKnowCapabilities )
    {
      getCapabilities();
    }
  if( !weKnowCapabilities )
    {
      return false;
    }
  if (s_camCapabilities.uiLowGain <= 0)
    {
      return false;
    }
  pNewTableLow = new FPROGAINVALUE[s_camCapabilities.uiLowGain];
  uiNumGainEntries = s_camCapabilities.uiLowGain;
  if (FPROSensor_GetGainTable(siDeviceHandle, FPRO_GAIN_TABLE_LOW_CHANNEL, pNewTableLow, &uiNumGainEntries) >= 0)
    {
      // check the index is not out of bounds
      if( gainIndex < uiNumGainEntries )
	{
	  // Set the gain index for low channel - need to retreive device index for given index first
	  FPROSensor_SetGainIndex(siDeviceHandle, FPRO_GAIN_TABLE_LOW_CHANNEL, pNewTableLow[gainIndex].uiDeviceIndex);
	  // Read it back - verification - should be the same as what was set
	  FPROSensor_GetGainIndex(siDeviceHandle, FPRO_GAIN_TABLE_LOW_CHANNEL, &uiGainDeviceIndex);
	  fGainValue = (float)pNewTableLow[gainIndex].uiValue / (float)FPRO_GAIN_SCALE_FACTOR;
	  printf( "Current low gain index setting is %d, which corresponds to device index %d and gain %5.3f\n",
		  gainIndex, uiGainDeviceIndex, fGainValue );
	  // return true only if read-back is identical
	  return (uiGainDeviceIndex == pNewTableLow[gainIndex].uiDeviceIndex);
	}
      else
	{
	  std::cerr << "FliCameraC::setLowGain() ERROR: index %d is out of range 0..%d"
		    << gainIndex << s_camCapabilities.uiLowGain << std::endl;	  
	  return false;
	}
    }
  else
    {
      std::cerr << "FliCameraC::setLowGain() ERROR: FPROSensor_GetGainTable() failed." << std::endl;
      return false;
    }
}

//--------------------------------------------------------------
/// return true if succeeded, false if failed
bool FliCameraC::setHighGain(uint32_t gainIndex)
{
  uint32_t uiGainDeviceIndex;
  uint32_t uiNumGainEntries;
  float    fGainValue;
  
  if( !weKnowCapabilities )
    {
      getCapabilities();
    }
  if( !weKnowCapabilities )
    {
      return false;
    }
  if (s_camCapabilities.uiHighGain <= 0)
    {
      return false;
    }
  pNewTableHigh = new FPROGAINVALUE[s_camCapabilities.uiHighGain];
  uiNumGainEntries = s_camCapabilities.uiHighGain;
  if (FPROSensor_GetGainTable(siDeviceHandle, FPRO_GAIN_TABLE_HIGH_CHANNEL, pNewTableHigh, &uiNumGainEntries) >= 0)
    {
      // check the index is not out of bounds
      if( gainIndex < uiNumGainEntries )
	{
	  // Set the gain index for high channel - need to retreive device index for given index first
	  FPROSensor_SetGainIndex(siDeviceHandle, FPRO_GAIN_TABLE_HIGH_CHANNEL, pNewTableHigh[gainIndex].uiDeviceIndex);
	  // Read it back - verification - should be the same as what was set
	  FPROSensor_GetGainIndex(siDeviceHandle, FPRO_GAIN_TABLE_HIGH_CHANNEL, &uiGainDeviceIndex);
	  fGainValue = (float)pNewTableHigh[gainIndex].uiValue / (float)FPRO_GAIN_SCALE_FACTOR;
	  printf( "Current high gain index setting is %d, which corresponds to device index %d and gain %5.3f\n",
		  gainIndex, uiGainDeviceIndex, fGainValue );
	  // return true only if read-back is identical
	  return (uiGainDeviceIndex == pNewTableHigh[gainIndex].uiDeviceIndex);
	}
      else
	{
	  std::cerr << "FliCameraC::setHighGain() ERROR: index %d is out of range 0..%d"
		    << gainIndex << s_camCapabilities.uiHighGain << std::endl;	  
	  return false;
	}	
    }
  else
    {
      std::cerr << "FliCameraC::setHighGain() ERROR: FPROSensor_GetGainTable() failed." << std::endl;
      return false;
    }
}

//--------------------------------------------------------------
/// print camera configuration
// return true if succeeded, false if failed
bool FliCameraC::getPrintConfig()
{
  int32_t iResult = -1;

  uint32_t colOffset = 0, rowOffset = 0, width = 0, height = 0;
  uint64_t exposureTime = 0, frameDelay = 0;
  bool immediate = false;
  bool isHdrEnabled = false;
  double temperatureSetPoint = 99.9;
  bool shutterUserControlPossible = 0;
  
  iResult = FPROFrame_GetImageArea( siDeviceHandle, &colOffset, &rowOffset, &width, &height );
  if( iResult < 0 )
    {
      std::cerr << "FliCameraC::getConfig() ERROR: FPROFrame_GetImageArea() failed, retval=" << iResult << std::endl;
    }
  
  iResult = FPROCtrl_GetExposure( siDeviceHandle, &exposureTime, &frameDelay, &immediate );
  if( iResult < 0 )
    {
      std::cerr << "FliCameraC::getConfig() ERROR: FPROFrame_GetExposure() failed, retval=" << iResult << std::endl;
    }

  iResult = FPROSensor_GetHDREnable( siDeviceHandle, &isHdrEnabled );
  if( iResult < 0 )
    {
      std::cerr << "FliCameraC::getConfig() ERROR: FPROFrame_GetHDREnable() failed, retval=" << iResult << std::endl;
    }

  iResult = FPROCtrl_GetTemperatureSetPoint( siDeviceHandle, &temperatureSetPoint );
  if( iResult < 0 )
    {
      std::cerr << "FliCameraC::getConfig() ERROR: FPROFrame_GetTemperatureSetPoint() failed, retval=" << iResult << std::endl;
    }

  getExternalTriggerEnable();

  shutterUserControlPossible = getShutterUserControl();
  printf("Camera configuration\n");
  printf("  Image Area\n");
  printf("    Row offset %5d\n", rowOffset);
  printf("    Col offset %5d\n", colOffset);
  printf("    Width      %5d\n", width);
  printf("    Height     %5d\n", height);
  printf("  Exposure [milliseconds]\n");
  printf("    Exposure time %13.6f\n", ((double)exposureTime)/1000000.0 );
  printf("    Frame delay   %13.6f\n", ((double)frameDelay)/1000000.0);
  printf("    Immediate      %d\n", (int)immediate );
  printf("  HDR mode enabled %d\n", (int)isHdrEnabled );
  
  printf("  Temperature set point (C) %6.2f\n", temperatureSetPoint );
  printf("  User control of mechnaical shutter possible   %d\n", shutterUserControlPossible );

  if( isExternalTriggerEnabled )
    {
      printf("  Extarnal trigger enabled, type %d ", externalTriggerType );
      switch( externalTriggerType )
	{
	case FLI_EXT_TRIGGER_FALLING_EDGE:
	  printf("(falling edge)\n");
	  break;
	case FLI_EXT_TRIGGER_RISING_EDGE:
	  printf("(raising edge)\n");
	  break;
	case FLI_EXT_TRIGGER_EXPOSE_ACTIVE_LOW:
	  printf("(active low)\n");
	  break;
	case FLI_EXT_TRIGGER_EXPOSE_ACTIVE_HIGH:
	  printf("(active high)\n");
	  break;
	default:
	  printf("(INVALID VALUE!)\n");	  
	}
    }
  else
    {
      printf("  Extarnal trigger disabled.\n");
    }

  if( getPixelConfig() )
    {
      printf("  Pixel bit depth (uiPixelDepth): %d\n", uiPixelDepth);
      printf("  Pixel offset (uiPixelLSB): %d\n", uiPixelLSB); 
    }

  if( getAllTemperatures() )
    {
      printAllTemperatures();
    }

  return( iResult >= 0 );
}

//--------------------------------------------------------------
/// return true if succeeded, false if failed
bool FliCameraC::prepareCaptureFullSensor()
{
  int32_t iResult;
  // assume failure
  iResult = -1;
  // Enable/disable image data
  // The power on default of the camera is to have image data enabled.
  // This is just shown here in case you are working with test frames and
  // need to set the camera back up to get you real image data.
  iResult = FPROFrame_SetImageDataEnable(siDeviceHandle, true);
  if( iResult < 0 )
    {
      std::cerr << "FliCameraC::prepareCaptureFullSensor() ERROR: FPROFrame_SetImageDataEnable() failed, retval=" << iResult << std::endl;
    }
  // Set the Image area
  // By default, the camera sets its image area to its maximum values.
  // For the GSENSE 400 model, that is 2048 columns x 2048 rows
  // But if you were to change the values this is how you would do it.
  //
  // MCU: why does GSENSE 4040 need 0, 0, 2048, 2048 and crashes with 0, 0, 4096, 4096,
  // which is it's netive resolution?
  if( iResult >= 0 )
    {
        iResult = FPROFrame_SetImageArea(siDeviceHandle, 0, 0, 4096, 4096);
    }
  else
    {
      std::cerr << "FliCameraC::prepareCaptureFullSensor() ERROR: FPROFrame_SetImageArea() failed, retval=" << iResult << std::endl;      
    }
  // return true if we are happy with iResult
  return( iResult >= 0 );
}

//--------------------------------------------------------------
/// return true if succeeded, false if failed
bool FliCameraC::allocFrameFullRes()
{
  // Make sure we have space for the image frame
  // A couple things to note about the frame size.
  //    1.) The macros we use here are supplied by the FPRO API but they
  //        only work if you are using 12 bit pixels.  If you change the pixel
  //        size you will need to do your own calculations to allocate memory.
  //    2.) ALL image frames come prepended with Meta Data.  The size is located in the
  //        capabilities structure we retrieved earlier.
  //        The format of the Meta Data is documented in your users manual.
  uiFrameSizeInBytes = s_camCapabilities.uiMetaDataSize + FLICAMERA_FRAME_SIZE_ADD_BULGARIAN_CONSTATNT +
    (FPRO_IMAGE_DIMENSIONS_TO_FRAMEBYTES(FLICAMERA_GSENSE4040_SENSOR_WIDTH, FLICAMERA_GSENSE4040_SENSOR_HEIGHT) * 2);

  printf("FliCameraC::allocFrameFullRes() DEBUG: Frame size (FPRO_IMAGE_DIMENSIONS_TO_FRAMEBYTES(%d, %d) = %d\n",
	 FLICAMERA_GSENSE4040_SENSOR_WIDTH, FLICAMERA_GSENSE4040_SENSOR_HEIGHT,
	 FPRO_IMAGE_DIMENSIONS_TO_FRAMEBYTES(FLICAMERA_GSENSE4040_SENSOR_WIDTH, FLICAMERA_GSENSE4040_SENSOR_HEIGHT) * 2 );
  printf("FliCameraC::allocFrameFullRes() DEBUG: uiFrameSizeInBytes = %d\n", uiFrameSizeInBytes);

  // free pframe in case it was allocated before (eg FliCameraC::allocFrameFullRes() not called 1st time
  // this is to prevent memory leak
  if( pFrame != NULL )
    {
      std::cout << "FliCameraC::allocFrameFullRes() Warning: pFrame is not NULL, assuming it was allocated before, try to free it... ";
      std::cout.flush();
      free( pFrame );
      std::cout << "OK" << std::endl;
    }
  // Now allocate 
  pFrame = (uint8_t *)malloc(uiFrameSizeInBytes);
  if( pFrame != NULL )
    {
      return true;
    }
  else
    {
      return false;
    }
}

//--------------------------------------------------------------
/// return true if succeeded, false if failed
bool FliCameraC::grabImage()
{
  int32_t  iResult = -1; // assuming failed
  int32_t  iGrabResult = -1; // assuming failed
  uint32_t  uiSizeGrabbed = 0;  // assuming zero bytes read
 
  // Start the capture and tell it to get 1 frame
  iResult = FPROFrame_CaptureStart(siDeviceHandle, 1);
  if (iResult >= 0)
    {
      // Grab  the frame- Here you can save the requested image size if you like as
      // the FPROFrame_GetVideoFrame() will return the actual number of bytes received.
      // Whatever you decide, make sure you pass the correct size into this API in order
      // to get the correct number of bytes for your frame.
      uiSizeGrabbed = uiFrameSizeInBytes;
      if( isExternalTriggerEnabled )
	{
	  iResult = FPROFrame_GetVideoFrameExt( siDeviceHandle, pFrame, &uiSizeGrabbed );
	}
      else
	{
	  iResult = FPROFrame_GetVideoFrame( siDeviceHandle, pFrame, &uiSizeGrabbed, 0 );
	}
      // Regardless of how the capture turned out, stop the capture
      iResult = FPROFrame_CaptureStop(siDeviceHandle);
      // If the FPROFrame_GetVideoFrame() succeeded- then process it
      if (iGrabResult >= 0)
	{
	  printf("FliCameraC::grabImage(): got a frame, %d bytes\n", uiSizeGrabbed);
	  // here you can do whatever you want with the frame data.
	  if (uiSizeGrabbed == uiFrameSizeInBytes)
	    {
	      printf("FliCameraC::grabImage(): Frame OK\n");
	      // got the correct number of bytes
	      
	      //pFrame;
	      
	    }
	  else
	    {
	      printf("FliCameraC::grabImage() ERROR: frame has different size than expected (%d bytes)\n", uiFrameSizeInBytes );
	      // something bad happened with the USB image transfer
	    }
	}
    }
  return (iGrabResult >= 0);
}

//--------------------------------------------------------------
/// return true if succeeded, false if failed
bool FliCameraC::startCapture(uint32_t num)
{
  int32_t  iResult = -1; // assuming failed
 
  // Start the capture and tell it to get num frames
  iResult = FPROFrame_CaptureStart(siDeviceHandle, num);
  if (iResult < 0)
    {
      std::cerr << "FliCameraC::startCapture(): FPROFrame_CaptureStart() failed, retval=" << iResult << std::endl;
    }
  return (iResult >= 0);
}

//--------------------------------------------------------------
/// return true if succeeded, false if failed
bool FliCameraC::getImage()
{
  int32_t  iResult = -1; // assuming failed
  uint32_t  uiSizeGrabbed = 0;  // assuming zero bytes read
  
  // Grab  the frame- Here you can save the requested image size if you like as
  // the FPROFrame_GetVideoFrame() will return the actual number of bytes received.
  // Whatever you decide, make sure you pass the correct size into this API in order
  // to get the correct number of bytes for your frame.
  uiSizeGrabbed = uiFrameSizeInBytes;
  // TODO: what does mean timeout 0? is infinite timeout?
  if( isExternalTriggerEnabled )
    {
      iResult = FPROFrame_GetVideoFrameExt( siDeviceHandle, pFrame, &uiSizeGrabbed );
      std::cout << "DEBUG: calling FPROFrame_GetVideoFrameExt\n";
    }
  else
    {
      iResult = FPROFrame_GetVideoFrame( siDeviceHandle, pFrame, &uiSizeGrabbed, 0 );
      //					 (this->exposureTime + this->frameDelay) / 1000000 + 1000 );
    }
  // If the FPROFrame_GetVideoFrame() succeeded- then process it
  if (iResult >= 0)
    {
      printf("FliCameraC::getImage(): Got a frame, %d bytes\n", uiSizeGrabbed);
      // here you can do whatever you want with the frame data.
      if (uiSizeGrabbed == uiFrameSizeInBytes)
	{
	  printf("FliCameraC::getImage(): frame OK\n");
	  // got the correct number of bytes
	  return true;
	}
      else
	{
	  printf("FliCameraC::getImage(): frame has different size than expected (%d bytes)\n", uiFrameSizeInBytes );
	  // something bad happened with the USB image transfer
	  return false;
	}
    }
  else
    {
      std::cerr << "FliCameraC::getImage(): FPROFrame_GetVideoFrame() failed, retval=" << iResult << std::endl;
      return false;
    }
}
  
//--------------------------------------------------------------
// end capture which just finished (why? unclear in lib documentation)
/// return true if succeeded, false if failed
bool FliCameraC::stopCapture()
{
  int32_t  iResult = -1; // assuming failed
  iResult = FPROFrame_CaptureStop(siDeviceHandle);
  if( iResult < 0 )
    {
      std::cerr << "FliCameraC::stopCapture() ERROR: FPROFrame_CaptureStop() failed, retval=" << iResult << std::endl;
      return false;
    }
  return true;    
}

//--------------------------------------------------------------
// interrupt capture while it is running (??? unclear in lib documentation)
// How does this differ from CaptureAbort()?
/// return true if succeeded, false if failed
bool FliCameraC::endCapture()
{
  int32_t  iResult = -1; // assuming failed
  iResult = FPROFrame_CaptureEnd(siDeviceHandle);
  if( iResult < 0 )
    {
      std::cerr << "FliCameraC::endCapture() ERROR: FPROFrame_CaptureEnd() failed, retval=" << iResult << std::endl;
      return false;
    }
  return true;    
}

//--------------------------------------------------------------
/// return true if succeeded, false if failed
bool FliCameraC::extractMetaData( uint8_t* pMetaData, uint32_t* metaDataSize )
{
  if( pFrame == NULL )
    {
      // there is apparently no frame captured
      return false;
    }
  pMetaData = pFrame;
  if( weKnowCapabilities )
    {
      *metaDataSize = s_camCapabilities.uiMetaDataSize;
      return true;
    }
  else
    {
      return false;
    }  
}
  
//--------------------------------------------------------------
/// return true if succeeded, false if failed
bool FliCameraC::convertHdrRawToBitmaps16bit( uint16_t* bitamp16bitLow, uint16_t* bitamp16bitHigh )
{
  if( pFrame == NULL )
    {
      // there is apparently no frame captured
      return false;
    }

  // MCu setup...
  uint8_t* rawPtrFromCamera = pFrame;
  uint32_t MetaDataSize = s_camCapabilities.uiMetaDataSize;
  uint32_t frameHeight = FLICAMERA_GSENSE4040_SENSOR_HEIGHT;
  uint32_t frameWidth = FLICAMERA_GSENSE4040_SENSOR_WIDTH; 
  uint32_t frameWidthBytes = (FLICAMERA_GSENSE4040_SENSOR_WIDTH * 3 / 2);
  uint8_t* tempRawLow;
  uint8_t* tempRawHigh = rawPtrFromCamera + MetaDataSize;  // Move 246 bytes to start at image data
  uint16_t* tempLdr = bitamp16bitLow;
  uint16_t* tempHdr = bitamp16bitHigh;

  // John's code
  // Now process the image data
  for (uint32_t y = 0; y < frameHeight; y++)
    {
      // In HDR mode, the capture contains both images interlaced so the pixels are:
      // (LDR, row0, pixel0), (LDR, row0, pixel1) ... (LDR, row0, pixelN),
      // (HDR, row0, pixel0), (HDR, row0, pixel1) ... (HDR, row0, pixelN),
      // (LDR, row1, pixel0), (LDR, row1, pixel1) ... (LDR, row1, pixelN),
      // (HDR, row1, pixel0), (HDR, row1, pixel1) ... (HDR, row1, pixelN),
      // (LDR, rowN, pixel0), (LDR, rowN, pixel1) ... (LDR, rowN, pixelN),
      // (HDR, rowN, pixel0), (HDR, rowN, pixel1) ... (HDR, rowN, pixelN),
      tempRawLow = tempRawHigh;   // Set the low raw pointer to where the High left off
      tempRawHigh += frameWidthBytes;  // Set the high one width past the Low
      
      // Since we are pulling off 2 pixels per loop only loop for half the width
      for (uint32_t x = 0; x < frameWidth; x += 2, tempRawLow += 3, tempRawHigh += 3)
	{
	  // Convert the Low values
	  uint8_t tL = tempRawLow[1];
	  tempLdr[0] = (uint16_t)((tempRawLow[0] << 4) | (tL >> 4));
	  tempLdr[1] = (uint16_t)(((tL & 0x0F) << 8) | tempRawLow[2]);

	  // Convert the High values
	  uint8_t tH = tempRawHigh[1];
	  tempHdr[0] = (uint16_t)((tempRawHigh[0] << 4) | (tH >> 4));
	  tempHdr[1] = (uint16_t)(((tH & 0x0F) << 8) | tempRawHigh[2]);
	  // Move the pointers
	  tempLdr += 2;
	  tempHdr += 2;	  
	}      
    }

  return true;
}

//--------------------------------------------------------------
/// return true if succeeded, false if failed
bool FliCameraC::convertLdrRawToBitmap16bit( uint16_t* bitamp16bit )
{
  if( pFrame == NULL )
    {
      // there is apparently no frame captured
      return false;
    }

  uint8_t* rawPtrFromCamera = pFrame;
  uint32_t MetaDataSize = s_camCapabilities.uiMetaDataSize;
  uint32_t frameHeight = FLICAMERA_GSENSE4040_SENSOR_HEIGHT;
  uint32_t frameWidth = FLICAMERA_GSENSE4040_SENSOR_WIDTH; 
  //  uint32_t frameWidthBytes = (FLICAMERA_GSENSE4040_SENSOR_WIDTH * 3 / 2);
  uint8_t* tempRaw = rawPtrFromCamera + MetaDataSize;  // Move 246 bytes to start at image data
  uint16_t* temp = bitamp16bit;

  // modifield John's code for LDR mode
  // Now process the image data
  for (uint32_t y = 0; y < frameHeight; y++)
    {
      // In HDR mode, the capture contains both images interlaced so the pixels are:
      // (LDR, row0, pixel0), (LDR, row0, pixel1) ... (LDR, row0, pixelN),
      // (LDR, row1, pixel0), (LDR, row1, pixel1) ... (LDR, row1, pixelN),
      // (LDR, rowN, pixel0), (LDR, rowN, pixel1) ... (LDR, rowN, pixelN),

      // Since we are pulling off 2 pixels per loop only loop for half the width
      for (uint32_t x = 0; x < frameWidth; x += 2, tempRaw += 3 )
	{
	  // Convert the Low values
	  uint8_t tL = tempRaw[1];
	  temp[0] = (uint16_t)((tempRaw[0] << 4) | (tL >> 4));
	  temp[1] = (uint16_t)(((tL & 0x0F) << 8) | tempRaw[2]);

	  temp += 2;
	}      
    }

  return true;
}

//--------------------------------------------------------------
/// return pointer to image bitmap
void* FliCameraC::getImagePtr()
{
  return (void *)(pFrame + s_camCapabilities.uiMetaDataSize + FLICAMERA_FRAME_SIZE_ADD_BULGARIAN_CONSTATNT);
}
