#include "libflipro.h"
#include "flicamera.h"
#include "flictl.h"

#include <fitsio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

/// check that 'opt1' and 'opt2' are not specified at the same time
void conflicting_options(const po::variables_map& vm, 
                         const char* opt1, const char* opt2)
{
    if (vm.count(opt1) && !vm[opt1].defaulted() 
        && vm.count(opt2) && !vm[opt2].defaulted())
      throw std::logic_error(std::string("Conflicting options '") 
                          + opt1 + "' and '" + opt2 + "'.");
}

/// just write a bitmap to fits file.
int writeFits(char *filename, int width, int height, void *data)
{
  FILE *pFile = fopen(filename, "r");
  if(pFile != NULL)
    {
      fclose(pFile);
      printf("\tFile '%s' already exists\n", filename);
      return 1;
    }
  
  int status = 0;
  long naxes[2] = {width, height};
  fitsfile *fp;
  
  fits_create_file(&fp, filename, &status);
  if(status)
    {
      printf("fits_create_file: %d\n", status);
      fits_report_error(stderr, status);
      return -1;
    }
  
  fits_create_img(fp, USHORT_IMG, 2, naxes, &status);
  if(status)
    {
      printf("fits_create_img: %d\n", status);
      fits_report_error(stderr, status);
      return -1;
    }
  
  fits_write_date(fp, &status);
  
  fits_write_img(fp, TUSHORT, 1, (width * height), data, &status);
  if(status)
    {
      printf("fits_write_img: %d\n", status);
      fits_report_error(stderr, status);
      return -1;
    }
  
  fits_close_file(fp, &status);
  
  if(status)
    {
      printf("fits_close_file: %d\n", status);
      fits_report_error(stderr, status);
      return -1;
    }
  
  return 0;
}

void exitCloseCameraDevice( FliCameraC* fc, int status )
{
  fc->closeDevice();
  exit( status );
}

//----------------------------------------------------
int main(int argc, const char ** argv)
{
  try
    {
      double coolTemp = 25.0;
      bool shutterOpen = false;
      bool isExtTriggerEnabled = false;
      // boost lib cannot work with enum FPROEXTTRIGTYPE externalTriggerType;
      // (well may be it does but not easily - to be investigated)
      uint32_t externalTriggerType;
      bool printCapabilities = false;
      bool printModes = false;
      //      uint32_t mode = 0;
      uint32_t lowGain = 0;
      uint32_t highGain = 0;
      uint32_t numImages = 1;
      // times are in nanoseconds
      uint64_t local_exposureTime = 500000000; // 0.5s
      uint64_t local_frameDelay = 1000000000; // 1s
      std::string fileNameBase = "fli_image";
      std::string fileName;
      
      po::options_description desc("flictl options");

#define EXT_TRIG_TYPE_HELP_TEXT "Set external trigger type\n\t" 
      
      char extTriggerHelpText[512];
      snprintf( extTriggerHelpText, 512, "Set external trigger type \n\t%d ... falling edge\n\t%d ... raising edge\n\t%d ... active low\n\t%d ... active high",
		FLI_EXT_TRIGGER_FALLING_EDGE, FLI_EXT_TRIGGER_RISING_EDGE,
		FLI_EXT_TRIGGER_EXPOSE_ACTIVE_LOW, FLI_EXT_TRIGGER_EXPOSE_ACTIVE_HIGH );
      
      desc.add_options()
	("help,h", "Print flictl help and exit.")
	("version,v", "Print flictl version and exit.")
	("printcap,p", po::bool_switch(&printCapabilities), "Print camera capabilities and temperatures")
	("printmodes", po::bool_switch(&printModes), "Print list of camera modes")
	//	("mode,m", po::value<uint32_t>(&mode), "Set cemare mode (index from list of modes)")
	("cool,c", po::value<double>(&coolTemp), "Set cooling temperature (Celsius degrees)")
	("shutter,s", po::value<bool>(&shutterOpen), "Shutter open (arg=1) or close (arg=0)")
	("trigger,t", po::value<bool>(&isExtTriggerEnabled), "Set trigger extrenal (arg=1) or internal (arg=0)")
	("exttrigtype,e", po::value<uint32_t>(&externalTriggerType), extTriggerHelpText)
	("lowgain", po::value<uint32_t>(&lowGain), "Set low gain")
	("highgain", po::value<uint32_t>(&highGain), "Set high gain")
	("exptime", po::value<uint64_t>(&local_exposureTime), "Set exposure time [nanoseconds]")
	("framedelay", po::value<uint64_t>(&local_frameDelay), "Set frame delay time [nanoseconds]")
	("getprintconfig", "Read configuration from camera and print it")
	("grabimage,g", "Grab single image and exit")
	("grabimages,G", po::value<uint32_t>(&numImages), "Grab N images and exit")
	("filename,f", po::value<std::string>(&fileNameBase), "Filename base for image(s) is captured.\n\tExample: -f file transfers into file_L.fit + file_H.fit (low gain and high gain images)");
	
      po::variables_map vm;

      po::store(po::parse_command_line(argc, argv, desc), vm);
      // no config file yet ... store(parse_config_file("example.cfg", desc), vm);
      notify(vm);

      if(vm.count("help"))
	{
	  /// Print help
	  std::cout << desc << std::endl;
	  exit( FLICTRL_OK );
	}
      if(vm.count("version"))
	{
	  /// Print program version
	  std::cout << "flictrl version : " << FLICTRL_VERSION << std::endl;
	  exit( FLICTRL_OK );
	}

      // Now we declare the camera class ans start initialising it
      FliCameraC fc; 
      
      // list camera devices and open first camera
      if( ! fc.listDevices() )
	{
	  exitCloseCameraDevice( &fc, FLICTRL_ERR_NO_CAMERA_FOUND );
	}
      if( ! fc.openDevice() )
	{
	  exitCloseCameraDevice( &fc, FLICTRL_ERR_CANNOT_OPEN_CAMERA_DEVICE );
	}
      fc.getCapabilities();
      fc.getPixelConfig(); 
      
      if(vm.count("printcap"))
	{
	  /// print camera capabilities
	  if( printCapabilities )
	    {
	      std::cout << "Print camera capabilities" << std::endl;
	    }
	}

      if(vm.count("cool"))
	{
	  /// Enable cooling - set temperature point
	  std::cout << "Cooling: set temperature = " << coolTemp << " ... ";

	  if( fc.setTemperatureSetPoint( coolTemp ) )
	    {
	      std::cout << " ... OK" << std::endl;
	    }
	  else
	    {
	      std::cout << " ... FAILED" << std::endl;
	    }	  
	}
      
      if(vm.count("shutter"))
	{
	  /// shutter open/close
	  if( shutterOpen )
	    {
	      std::cout << "Open mechanical shutter... ";
	    }
	  else
	    {
	      std::cout << "Close mechanical shutter... ";
	    }
	  if( ! fc.getShutterUserControl() )
	    {
	      if( ! fc.setShutterUserControl(true) )
		{
		  exitCloseCameraDevice( &fc, FLICTRL_ERR );
		}
	    }
	  if( fc.setShutter( shutterOpen ) )
	    {
	      std::cout << " ... OK" << std::endl;
	    }
	  else
	    {
	      std::cout << " ... FAILED" << std::endl;
	    }
	}

      if(vm.count("trigger"))
	{
	  /// select external trigger or internal trigger
	  if( ! vm.count("exttrigtype") )
	    {
	      bool dummy;
	      fc.getExternalTriggerEnable( &dummy, (FPROEXTTRIGTYPE*)(&externalTriggerType) );
	    }
	  if( ! fc.setExternalTriggerEnable( isExtTriggerEnabled, (FPROEXTTRIGTYPE)externalTriggerType ) )
	    {
	      exitCloseCameraDevice( &fc, FLICTRL_ERR );
	    }	  
	}
      
      if(vm.count("lowgain"))
	{
	  /// set low gain
	  std::cout << "DEBUG: set lowgain list index = " << lowGain << std::endl;
	}
      
      if(vm.count("highgain"))
	{
	  /// set high gain
	  std::cout << "DEBUG: set highgain list index = " << highGain << std::endl;
	}

      if(vm.count("filename"))
	{
	  fileNameBase = vm["filename"].as<std::string>();
	  std::cout << "DEBUG: set file name base = " << fileNameBase << std::endl;
	}
      
      //--------------------------------------
     
      if( printCapabilities )
	{
	  fc.printCapabilities();
	}

      if( printModes )
	{
	  if( ! fc.printModes() )
	    {
	      exitCloseCameraDevice( &fc, FLICTRL_ERR );
	    }	  	    
	}

      if(vm.count("exptime"))
	{
	  if( ! fc.setExpTime(local_exposureTime) )
	    {
	      exitCloseCameraDevice( &fc, FLICTRL_ERR );
	    }
	}

      if(vm.count("framedelay"))
	{
	  if( ! fc.setExpDelay(local_frameDelay) )
	    {
	      exitCloseCameraDevice( &fc, FLICTRL_ERR );
	    }
	}

      if(vm.count("lowgain"))
	{
	  if( ! fc.setLowGain(lowGain) )
	    {
	      exitCloseCameraDevice( &fc, FLICTRL_ERR );
	    }
	}

      if(vm.count("highgain"))
	{
	  if( ! fc.setHighGain(highGain) )
	    {
	      exitCloseCameraDevice( &fc, FLICTRL_ERR );
	    }
	}

      // run print config after all configurations
      if(vm.count("getprintconfig"))
	{
	  if( ! fc.getPrintConfig() )
	    {
	      exitCloseCameraDevice( &fc, FLICTRL_ERR );
	    }	  
	}

      //------------------------------------------------
      if(vm.count("grabimage"))
	{
	  /// Grab single image and exit
	  if( ! fc.allocFrameFullRes() )
	    {
	      exitCloseCameraDevice( &fc, FLICTRL_ERR_FAILED_ALLOC_FRAME );
	    }
	  if( ! fc.prepareCaptureFullSensor() )
	    {
	      exitCloseCameraDevice( &fc, FLICTRL_ERR );
	    }
	  if( ! fc.grabImage() )
	    {
	      exitCloseCameraDevice( &fc, FLICTRL_ERR_FAILED_GRAB_IMAGE );
	    }
/* single (LDR only) image	  
	  uint16_t* bitmap16bit = new uint16_t[ FLICAMERA_GSENSE4040_SENSOR_HEIGHT * FLICAMERA_GSENSE4040_SENSOR_WIDTH ];

	  fc.convertLdrRawToBitmap16bit( bitmap16bit );
	  
	  if(vm.count("filename"))	    
	    {
	      fileNameBase = vm["filename"].as<std::string>();
	      fileName = filenameBase + "_H.fit";
	      std::cout << "Write image to as " << fileName << std::endl;
	      char *fName = &fileName[0u];
	      writeFits( fName,
			 FLICAMERA_GSENSE4040_SENSOR_WIDTH,
			 FLICAMERA_GSENSE4040_SENSOR_HEIGHT,
			 bitmap16bit );
	    }
	  
	  delete [] bitmap16bit;
*/

	  uint16_t* bitmap16bitL = new uint16_t[ FLICAMERA_GSENSE4040_SENSOR_HEIGHT * FLICAMERA_GSENSE4040_SENSOR_WIDTH ];
	  uint16_t* bitmap16bitH = new uint16_t[ FLICAMERA_GSENSE4040_SENSOR_HEIGHT * FLICAMERA_GSENSE4040_SENSOR_WIDTH ];

	  fc.convertHdrRawToBitmaps16bit( bitmap16bitL, bitmap16bitH );
	  char *fName;
	  
	  fileName = fileNameBase + "_L.fit";
	  std::cout << "Write low gain image to as " << fileName << std::endl;
	  fName = &fileName[0u];
	  // TODO: check retval of writeFits
	  writeFits( fName,
		     FLICAMERA_GSENSE4040_SENSOR_WIDTH,
		     FLICAMERA_GSENSE4040_SENSOR_HEIGHT,
		     bitmap16bitL );

	  fileName = fileNameBase + "_H.fit";
	  std::cout << "Write high gain image to as " << fileName << std::endl;
	  fName = &fileName[0u];
	  // TODO: check retval of writeFits
	  writeFits( fName,
		     FLICAMERA_GSENSE4040_SENSOR_WIDTH,
		     FLICAMERA_GSENSE4040_SENSOR_HEIGHT,
		     bitmap16bitH );
	  delete [] bitmap16bitL;
	  delete [] bitmap16bitH;
	  // all good exit witout error
	  exitCloseCameraDevice( &fc, FLICTRL_OK );
	}
      
      //------------------------------------------------
      if(vm.count("grabimages"))
	{
	  /// Grab N images and exit

	  if( ! fc.allocFrameFullRes() )
	    {
	      exitCloseCameraDevice( &fc, FLICTRL_ERR_FAILED_ALLOC_FRAME );
	    }
	  if( ! fc.prepareCaptureFullSensor() )
	    {
	      exitCloseCameraDevice( &fc, FLICTRL_ERR );
	    }

	  if( ! isExtTriggerEnabled )
	    {
	      // call startCapture only when not triggering externally
	      // (when using FLI camera internal trigger)
	      if( ! fc.startCapture(numImages) )
		{
		  // TODO - define specific err code
		  exitCloseCameraDevice( &fc, FLICTRL_ERR );
		}
	    }

	  uint16_t* bitmap16bitL = new uint16_t[ FLICAMERA_GSENSE4040_SENSOR_HEIGHT * FLICAMERA_GSENSE4040_SENSOR_WIDTH ];
	  uint16_t* bitmap16bitH = new uint16_t[ FLICAMERA_GSENSE4040_SENSOR_HEIGHT * FLICAMERA_GSENSE4040_SENSOR_WIDTH ];

	  uint32_t numDigits = 5;
	  char numberStr[numDigits + 1];
	  char *fName;
	  
	  for( uint32_t i=0; i<numImages; i++ )
	    {
	      if( isExtTriggerEnabled )
		{
		  std::cout << "Waiting for external trigger... ";
		}
	      std::cout << "Image # " << i << std::endl;
		
	      if( ! fc.getImage() )
		{
		  // TODO - define specific err code
		  exitCloseCameraDevice( &fc, FLICTRL_ERR );
		}		
	      fc.convertHdrRawToBitmaps16bit( bitmap16bitL, bitmap16bitH );
	      // TODO: replace "%05d" with something using numDigits
	      snprintf( numberStr, numDigits+1, "%05d", i );
	      // TODO: add time to the file name - as for DFN cameras
	      fileName = fileNameBase + numberStr + "_L.fit";
	      std::cout << "  Write low gain image to as " << fileName << std::endl;
	      fName = &fileName[0u];
	      // TODO: check retval of writeFits
	      writeFits( fName,
			 FLICAMERA_GSENSE4040_SENSOR_WIDTH,
			 FLICAMERA_GSENSE4040_SENSOR_HEIGHT,
			 bitmap16bitL );
	      
	      fileName = fileNameBase + numberStr + "_H.fit";
	      std::cout << "  Write high gain image to as " << fileName << std::endl;
	      fName = &fileName[0u];
	      // TODO: check retval of writeFits
	      writeFits( fName,
			 FLICAMERA_GSENSE4040_SENSOR_WIDTH,
			 FLICAMERA_GSENSE4040_SENSOR_HEIGHT,
			 bitmap16bitH );
	      
	    }
	  delete [] bitmap16bitL;
	  delete [] bitmap16bitH;

	  if( isExtTriggerEnabled )
	    {
	      // disable external triggering (set camera to use internal triggering),
	      // but keep the previously set ext trigger type
	      bool dummy;
	      fc.getExternalTriggerEnable( &dummy, (FPROEXTTRIGTYPE*)(&externalTriggerType) );	      
	      if( ! fc.setExternalTriggerEnable( false, (FPROEXTTRIGTYPE)externalTriggerType ) )
		{
		  exitCloseCameraDevice( &fc, FLICTRL_ERR );
		}	  
	    }
	  else
	    {
	      // call stopCapture only when not triggering externally
	      // (when using FLI camera internal trigger)
	      if( ! fc.stopCapture() )
		{
		  // TODO - define specific err code
		  exitCloseCameraDevice( &fc, FLICTRL_ERR );
		}
	    }
	  // all good exit witout error
	  exitCloseCameraDevice( &fc, FLICTRL_OK );
        }
    }
  catch(std::exception& e)
    {
      std::cerr << "Error parsing options!" << std::endl;
      std::cerr << e.what() << std::endl;
    }  
}
