#include "libflipro.h"
#include "flicamera.h"
#include "flictl.h"

#include <boost/program_options.hpp>
//#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <fitsio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>

namespace po = boost::program_options;

void printVersion()
{
  std::cout << FLICTL_APP_DESCR << " version " << FLICTL_VERSION << std::endl;
}

/// check that 'opt1' and 'opt2' are not specified at the same time
void conflicting_options(const po::variables_map& vm, 
                         const char* opt1, const char* opt2)
{
    if (vm.count(opt1) && !vm[opt1].defaulted() 
        && vm.count(opt2) && !vm[opt2].defaulted())
      throw std::logic_error(std::string("Conflicting options '") 
                          + opt1 + "' and '" + opt2 + "'.");
}

/// write FLI camera meta data binary blob to file - as it is
int writeMetaData( const char *filename, uint8_t *mem, uint32_t memSize )
{
  std::ofstream os( filename, std::ofstream::binary );
  if( ! os.is_open() ) 
    {
      // show message:
      std::cerr << "writeMetaData(): Error opening file " << filename << std::endl;
      return -1;
    }
    
  os.write( (char *)mem, memSize );
  os.flush();
  os.close();

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
      bool isTimeInFileNames = false;
      bool doWriteMetaData = false;
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
      // the default values are what camera configuretion dump reports after camera power-up
      uint64_t local_exposureTime = 2000000; // 2ms aka 1/500s
      uint64_t local_frameDelay = 0; // 0s
      std::string fileNameBase = "fli_image_";
      std::string fileName;
      std::string siteLocation = "default_lab";

      // default coordinates - Perth, Curtin Bentley campus
      double latitude = -32.00720;
      double longitude = 115.89469;
      double altitude = 50.0;

      boost::posix_time::ptime ptime_frameTimeStamp;
      boost::posix_time::ptime ptime_truncFrameTimeStamp;
      boost::posix_time::ptime ptime_roundFrameTimeStamp;
      
      boost::posix_time::ptime ptime_fileNameFrameTimeStamp;
      std::string str_fileNameFrameTimeStamp = "";
      
      po::options_description desc("Usage:\n flictl [options]\n\nOptions");

#define EXT_TRIG_TYPE_HELP_TEXT "Set external trigger type\n\t" 
      
      char extTriggerHelpText[512];
      snprintf( extTriggerHelpText, 512, "Set external trigger type \n\t%d ... falling edge\n\t%d ... raising edge\n\t%d ... active low\n\t%d ... active high",
		FLI_EXT_TRIGGER_FALLING_EDGE, FLI_EXT_TRIGGER_RISING_EDGE,
		FLI_EXT_TRIGGER_EXPOSE_ACTIVE_LOW, FLI_EXT_TRIGGER_EXPOSE_ACTIVE_HIGH );
      
      desc.add_options()
	("help,h", "Print flictl help and exit.")
	("version,V", "Print flictl version and exit.")
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
	("filename,f", po::value<std::string>(&fileNameBase), "Filename base for image(s) is captured.\n\tExample: -f file transfers into file_L_fli.fits + file_H_fli.fits (low gain and high gain images)")
	("time",  po::bool_switch(&isTimeInFileNames), "Add exposure start time to filename(s)" )
	("meta",  po::bool_switch(&doWriteMetaData), "Write binary meta data to extra file" )
	("lat", po::value<double>(&latitude), "Set latitude that will be written into FITS header [double, decimal degrees]")
	("lon", po::value<double>(&longitude), "Set longitude that will be written into FITS header [double, decimal degrees]")
	("alt", po::value<double>(&altitude), "Set altitude that will be written into FITS header [double, meters]")
	("location", po::value<std::string>(&siteLocation), "Set location name that will be written into FITS header.");
      
      po::variables_map vm;

      po::store(po::parse_command_line(argc, argv, desc), vm);
      // no config file yet ... store(parse_config_file("example.cfg", desc), vm);
      notify(vm);

      // dunno how to detect that there are no parameters with boost::program_options
      if( (argc <= 1)  || vm.count("help") )
	{
	  /// Print help
	  printVersion();
	  std::cout << desc << std::endl;
	  exit( FLICTL_OK );
	}
      if(vm.count("version"))
	{
	  /// Print program version
	  printVersion();
	  exit( FLICTL_OK );
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

      if( vm.count("time") )
	{
	  std::cout << "DEBUG: add exposure start time to image file name(s) = " << isTimeInFileNames << std::endl;
	}

      if( vm.count("meta") )
	{
	  std::cout << "DEBUG: write meta data to extra file = " << doWriteMetaData << std::endl;
	}

      if( vm.count("lat") )
	{
	  std::cout << "DEBUG: latitude = " << latitude << " will be written to FITS file header." << std::endl;
	}

      if( vm.count("lon") )
	{
	  std::cout << "DEBUG: longitude = " << longitude << " will be written to FITS file header."  << std::endl;
	}

      if( vm.count("alt") )
	{
	  std::cout << "DEBUG: altitude = " << altitude << " will be written to FITS file header."  << std::endl;
	}

      if( vm.count("location") )
	{
	  std::cout << "DEBUG: site location = " << siteLocation << " will be written to FITS file header."  << std::endl;
	}

      if( vm.count("filename") )
	{
	  if( isTimeInFileNames )
	    {
	      ptime_frameTimeStamp = boost::posix_time::microsec_clock::universal_time();
	      //std::cout << "DEBUG: frameTimeStamp read time = " << ptime_frameTimeStamp << std::endl;
	      ptime_frameTimeStamp -= boost::posix_time::nanoseconds(local_exposureTime);
	      //std::cout << "DEBUG: frameTimeStamp start exp time = " << ptime_frameTimeStamp << std::endl;
	      //std::cout << "DEBUG: frameTimeStamp start exp time truncated nanoseconds = "
	      //	<< ptime_frameTimeStamp.time_of_day().total_nanoseconds() % 1000000000 << std::endl;

	       if( isExtTriggerEnabled )
		 {
		   ptime_truncFrameTimeStamp = ptime_frameTimeStamp
		     - boost::posix_time::nanoseconds( ptime_frameTimeStamp.time_of_day().total_nanoseconds() % 1000000000 );
		   str_fileNameFrameTimeStamp = "_" + to_iso_string( ptime_truncFrameTimeStamp );
		   std::cout << "DEBUG: external trigger frameTimeStamp string example: " << str_fileNameFrameTimeStamp << std::endl;

		 }
	       else
		 {
		   uint32_t extraSec = (( ptime_frameTimeStamp.time_of_day().total_nanoseconds() % 1000000000) >= 500000000);
		   ptime_roundFrameTimeStamp = ptime_frameTimeStamp
		     - boost::posix_time::nanoseconds( ptime_frameTimeStamp.time_of_day().total_nanoseconds() % 1000000000 )
		     + boost::posix_time::seconds(extraSec);
		   str_fileNameFrameTimeStamp = "_" + to_iso_string( ptime_roundFrameTimeStamp );
		   
		   std::cout << "DEBUG: internal trigger frameTimeStamp string example: " << str_fileNameFrameTimeStamp << std::endl;
		 }
	    }
	  
	  fileNameBase = vm["filename"].as<std::string>();
	  std::cout << "DEBUG: set file name base = " << fileNameBase << std::endl;
	}

      if(vm.count("printcap"))
	{
	  /// print camera capabilities
	  if( printCapabilities )
	    {
	      std::cout << "Print camera capabilities" << std::endl;
	    }
	}
      
      // ---------------------------------------------------------------
      // Now we declare the camera class and start initialising it
      FliCameraC fc; 
      
      // list camera devices and open first camera
      if( ! fc.listDevices() )
	{
	  std::cerr << argv[0] << ": fc.listDevices() Failed!" << std::endl;
	  exitCloseCameraDevice( &fc, FLICTL_ERR_NO_CAMERA_FOUND );
	}
      if( ! fc.openDevice() )
	{
	  std::cerr << argv[0] << ": fc.openDevice() Failed!" << std::endl;
	  exitCloseCameraDevice( &fc, FLICTL_ERR_CANNOT_OPEN_CAMERA_DEVICE );
	}
      fc.getCapabilities();
      fc.getPixelConfig(); 
      
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
		  exitCloseCameraDevice( &fc, FLICTL_ERR );
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
	      exitCloseCameraDevice( &fc, FLICTL_ERR );
	    }	  
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
	      exitCloseCameraDevice( &fc, FLICTL_ERR );
	    }	  	    
	}

      if(vm.count("exptime"))
	{
	  if( ! fc.setExpTime(local_exposureTime) )
	    {
	      exitCloseCameraDevice( &fc, FLICTL_ERR );
	    }
	}

      if(vm.count("framedelay"))
	{
	  if( ! fc.setExpDelay(local_frameDelay) )
	    {
	      exitCloseCameraDevice( &fc, FLICTL_ERR );
	    }
	}

      if(vm.count("lowgain"))
	{
	  if( ! fc.setLowGain(lowGain) )
	    {
	      exitCloseCameraDevice( &fc, FLICTL_ERR );
	    }
	}

      if(vm.count("highgain"))
	{
	  if( ! fc.setHighGain(highGain) )
	    {
	      exitCloseCameraDevice( &fc, FLICTL_ERR );
	    }
	}

      // run print config after all configurations
      if(vm.count("getprintconfig"))
	{
	  if( ! fc.getPrintConfig() )
	    {
	      exitCloseCameraDevice( &fc, FLICTL_ERR );
	    }	  
	}

      //------------------------------------------------
      if(vm.count("grabimage"))
	{
	  std::cout << "Grab single image and exit. " << std::endl;
	  /// Grab single image and exit
	  numImages = 1;
	}
      
      //------------------------------------------------
      if(vm.count("grabimages") || vm.count("grabimage"))
	{
	  /// Grab N images and exit

	  if( ! fc.allocFrameFullRes() )
	    {
	      exitCloseCameraDevice( &fc, FLICTL_ERR_FAILED_ALLOC_FRAME );
	    }
	  if( ! fc.prepareCaptureFullSensor() )
	    {
	      exitCloseCameraDevice( &fc, FLICTL_ERR );
	    }

	  if( ! isExtTriggerEnabled )
	    {
	      // call startCapture only when not triggering externally
	      // (when using FLI camera internal trigger)
	      if( ! fc.startCapture(numImages) )
		{
		  // TODO - define specific err code
		  exitCloseCameraDevice( &fc, FLICTL_ERR );
		}
	    }

	  uint16_t* bitmap16bitL = new uint16_t[ FLICAMERA_GSENSE4040_SENSOR_HEIGHT * FLICAMERA_GSENSE4040_SENSOR_WIDTH ];
	  uint16_t* bitmap16bitH = new uint16_t[ FLICAMERA_GSENSE4040_SENSOR_HEIGHT * FLICAMERA_GSENSE4040_SENSOR_WIDTH ];

	  uint32_t metaDataSize;
	  fc.getMetaDataSize( &metaDataSize );
	  uint8_t* metaDataBinBlob = new uint8_t[ metaDataSize ];
	    
	  uint32_t numDigits = 5;
	  char numberStr[numDigits + 1];
	  char *fName;

	  fc.setFitsLocation( latitude, longitude, altitude, siteLocation );
	  
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
		  exitCloseCameraDevice( &fc, FLICTL_ERR );
		}

	      fc.getLastFrameTimeStamp( &ptime_fileNameFrameTimeStamp );
	      str_fileNameFrameTimeStamp = "_" + to_iso_string( ptime_fileNameFrameTimeStamp );
		
	      if( isTimeInFileNames )
		{
		  if( isExtTriggerEnabled )
		    {
		      std::cout << "  Externally triggered frame capture time = "
				<< str_fileNameFrameTimeStamp << std::endl;
		    }
		  else
		    {		      
		      std::cout << "  Approx internally triggered frame capture time = "
				<< str_fileNameFrameTimeStamp << std::endl;
		    }		    
		}
	      fc.convertHdrRawToBitmaps16bit( bitmap16bitL, bitmap16bitH );	      
	      // TODO: replace "%05d" with something using numDigits
	      snprintf( numberStr, numDigits+1, "%05d", i );
	      // TODO: add time to the file name - as for DFN cameras
	      fileName = fileNameBase + numberStr + str_fileNameFrameTimeStamp + "_L_fli.fits";
	      std::cout << "  Write low gain image as " << fileName << std::endl;
	      fName = &fileName[0u];
	      // TODO: check retval of writeFits	      
	      fc.writeFits( fName,
			    FLICAMERA_GSENSE4040_SENSOR_WIDTH,
			    FLICAMERA_GSENSE4040_SENSOR_HEIGHT,
			    bitmap16bitL, 'L' );
	      
	      fileName = fileNameBase + numberStr + str_fileNameFrameTimeStamp + "_H_fli.fits";
	      std::cout << "  Write high gain image as " << fileName << std::endl;
	      fName = &fileName[0u];
	      // TODO: check retval of writeFits
	      fc.writeFits( fName,
			 FLICAMERA_GSENSE4040_SENSOR_WIDTH,
			 FLICAMERA_GSENSE4040_SENSOR_HEIGHT,
			    bitmap16bitH, 'H' );

	      if( doWriteMetaData )
		{
		  // write meta data binary blob
		  fc.extractMetaData( metaDataBinBlob, metaDataSize );
		  fileName = fileNameBase + numberStr + str_fileNameFrameTimeStamp + "_meta.bin";
		  std::cout << "  Write binary meta data as " << fileName << std::endl;
		  fName = &fileName[0u];
		  writeMetaData( fName, metaDataBinBlob, metaDataSize );
		}
	    }
	  delete [] bitmap16bitL;
	  delete [] bitmap16bitH;
	  delete [] metaDataBinBlob;
	  
	  if( isExtTriggerEnabled )
	    {
	      // disable external triggering (set camera to use internal triggering),
	      // but keep the previously set ext trigger type
	      bool dummy;
	      fc.getExternalTriggerEnable( &dummy, (FPROEXTTRIGTYPE*)(&externalTriggerType) );	      
	      if( ! fc.setExternalTriggerEnable( false, (FPROEXTTRIGTYPE)externalTriggerType ) )
		{
		  exitCloseCameraDevice( &fc, FLICTL_ERR );
		}	  
	    }
	  else
	    {
	      // call stopCapture only when not triggering externally
	      // (when using FLI camera internal trigger)
	      if( ! fc.stopCapture() )
		{
		  // TODO - define specific err code
		  exitCloseCameraDevice( &fc, FLICTL_ERR );
		}
	    }
	  // all good exit witout error
	  exitCloseCameraDevice( &fc, FLICTL_OK );
        }
    }
  catch(std::exception& e)
    {
      std::cerr << "Error parsing options!" << std::endl;      
      std::cerr << e.what() << std::endl;
      std::cerr << "Use 'flictl -h' to het help" << std::endl;      
    }  
}
