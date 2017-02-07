//============================================================================
// Name        : multiplecam.cpp
// Author      : Sara Turriziani
// Version     : 2.1
// Copyright   : Mini-EUSO copyright notice
// Description : Cameras Acquisition Module in C++, ANSI-style, for linux
//============================================================================

#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <sstream>
#include <fstream>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "/usr/include/flycapture/FlyCapture2.h"
using namespace FlyCapture2;
using namespace std;

unsigned createMask(unsigned a, unsigned b)
{
	unsigned int r = 0;
	for (unsigned i = a; i < b; i++)
	{
		r = r+1;
		r  = r*2;
	}
	r = r + 1;
	return r;
}

// test time in ms
const std::string currentDateTime2() {
 timeval curTime;
 gettimeofday(&curTime, NULL);
 int milli = curTime.tv_usec / 1000;
 struct tm timeinfo;
 char buffer [80];
 strftime(buffer, sizeof(buffer), "%Y-%m-%d.%H-%M-%S", localtime_r(&curTime.tv_sec, &timeinfo));
 char currentTime[84] = "";
 sprintf(currentTime, "%s.%03d", buffer, milli);

 return currentTime;

  }

void PrintBuildInfo()
{
    FC2Version fc2Version;
    Utilities::GetLibraryVersion( &fc2Version );
    char version[128];
    sprintf(
        version,
        "FlyCapture2 library version: %d.%d.%d.%d\n",
        fc2Version.major, fc2Version.minor, fc2Version.type, fc2Version.build );

    printf( version );

    char timeStamp[512];
    sprintf( timeStamp, "Application build date: %s %s\n\n", __DATE__, __TIME__ );

    printf( timeStamp );
}

void PrintCameraInfo( CameraInfo* pCamInfo )
{
    printf(
        "\n*** CAMERA INFORMATION ***\n"
        "Serial number - %u\n"
        "Camera model - %s\n"
        "Camera vendor - %s\n"
        "Sensor - %s\n"
        "Resolution - %s\n"
        "Firmware version - %s\n"
        "Firmware build time - %s\n\n",
        pCamInfo->serialNumber,
        pCamInfo->modelName,
        pCamInfo->vendorName,
        pCamInfo->sensorInfo,
        pCamInfo->sensorResolution,
        pCamInfo->firmwareVersion,
        pCamInfo->firmwareBuildTime );
}

void PrintError( Error error )
{
    error.PrintErrorTrace();
}

enum ExtendedShutterType
{
    NO_EXTENDED_SHUTTER,
    GENERAL_EXTENDED_SHUTTER
};

//int main(int /*argc*/, char** /*argv*/)
int main()
{
	const int k_numImages = 100;
	unsigned int ulValue;
	CameraInfo camInfo;

    PrintBuildInfo();
    Error error;
    // Since this application saves images in the current folder
    // we must ensure that we have permission to write to this folder.
    // If we do not have permission, fail right away.
	FILE* tempFile = fopen("test.txt", "w+");
	if (tempFile == NULL)
	{
		printf("Failed to create file in current folder.  Please check permissions.\n");
		return -1;
	}
	fclose(tempFile);
	remove("test.txt");

    BusManager busMgr;
    unsigned int numCameras;
    error = busMgr.GetNumOfCameras(&numCameras);
    if (error != PGRERROR_OK)
    {
        PrintError( error );
        return -1;
    }

    printf( "Number of cameras detected: %u\n", numCameras );

    Camera *pCameras = new Camera[numCameras]; // initialize an array of cameras

    for (unsigned int i=0; i < numCameras; i++)
          {
            PGRGuid guid;
            error = busMgr.GetCameraFromIndex(i, &guid);
            if (error != PGRERROR_OK)
             {
                PrintError( error );
                delete[] pCameras;
                return -1;
             }
            error = pCameras[i].Connect(&guid); // connect both cameras
            if (error != PGRERROR_OK)
              {
                PrintError(error);
                delete[] pCameras;
                return -1;
               }

          error = pCameras[i].GetCameraInfo(&camInfo);
          if (error != PGRERROR_OK)
           {
             PrintError(error);
             delete[] pCameras;
             return -1;
            }

          PrintCameraInfo(&camInfo);

          // Turn Timestamp on

          EmbeddedImageInfo imageInfo;
          imageInfo.timestamp.onOff = true;
          error = pCameras[i].SetEmbeddedImageInfo(&imageInfo);
          if (error != PGRERROR_OK)
           {
             PrintError(error);
             delete[] pCameras;
             return -1;
           }

          // read from the camera register the default factory value for each parameter

          Property frmRate;
          frmRate.type = FRAME_RATE;
          error = pCameras[i].GetProperty( &frmRate );
          if (error != PGRERROR_OK)
            {
              PrintError( error );
              delete[] pCameras;
              return -1;
            }

          PropertyInfo frRate;
          frRate.type = FRAME_RATE;
          error = pCameras[i].GetPropertyInfo( &frRate );
          if (error != PGRERROR_OK)
            {
              PrintError( error );
              delete[] pCameras;
              return -1;
            }
          float minfrmRate = frRate.absMin;
          float maxfrmRate = frRate.absMax;

          printf( "Default Frame rate is %3.1f fps. \n" , frmRate.absValue );

          if (camInfo.serialNumber == 15568736) // read the temperature registe ronly for NIR camera
            {
              error = pCameras[i].ReadRegister(0x82C, &ulValue); // read the temperature register
              if (error != PGRERROR_OK)
                {
                  PrintError( error );
                  delete[] pCameras;
                  return -1;
                }

              unsigned r = createMask(20, 31); // extract the bits of interest
              unsigned int res = r & ulValue;  // here you have already the Kelvin temperature * 10
              cout << "Initial Temperature is " << res / 10.0 << " K/ " << res / 10.0	- 273.15 << " Celsius" << endl;
          }

          //Update the register of each camera reading from parfile

          std::string line;
          float frate, shutt, autexpo;

          ifstream parfile ("/home/minieusouser/CPU/cameras/test/test.ini");

          // Set the new values from the parameters reading them from the parameter file
          if (parfile.is_open())
           {
              printf( "Reading from parfile: \n" );
              while ( getline (parfile,line) )
                   {
                     std::istringstream in(line);      //make a stream for the line itself
                     std::string type;
                     in >> type;                  //and read the first whitespace-separated token

                     if(type == "FRAMERATE")       //and check its value
                      {
                        in >> frate;       //now read the whitespace-separated floats
                        cout << type << " " << frate << " fps " << endl;
                      }
                     else if(type == "SHUTTER")
                      {
                        in >> shutt;
                        cout << type << " " << shutt << " ms " << endl;
                      }
                     else if((type == "AUTOEXPOSURE"))
                      {
                        in >> autexpo;
                        cout << type << " " << autexpo << endl;
                      }
                }
            parfile.close();

           }
            else
                {
                 printf( "Unable to open parfile!!! \n" );
                 return -1;
                }


            // Turn off autoexposure
            Property prop;
            prop.type = AUTO_EXPOSURE;
            prop.autoManualMode = false;
            prop.onOff = false;

            error = pCameras[i].SetProperty( &prop );
            if (error != PGRERROR_OK)
              {
                PrintError( error );
                delete[] pCameras;
                return -1;
              }

            // To turn off frame rate and enable extended shutter
            // first of all, check if the camera supports the FRAME_RATE property
                PropertyInfo propInfo;
                propInfo.type = FRAME_RATE;
                error =  pCameras[i].GetPropertyInfo( &propInfo );
                if (error != PGRERROR_OK)
                {
                    PrintError( error );
                    delete[] pCameras;
                    return -1;
                }

             ExtendedShutterType shutterType = NO_EXTENDED_SHUTTER;

             if ( propInfo.present == true )
              {
                // Then turn off frame rate

                Property prop;
                prop.type = FRAME_RATE;
                error =  pCameras[i].GetProperty( &prop );
                if (error != PGRERROR_OK)
                  {
                    PrintError( error );
                    delete[] pCameras;
                    return -1;
                  }

                prop.autoManualMode = false;
                prop.onOff = false;

                error =  pCameras[i].SetProperty( &prop );
                if (error != PGRERROR_OK)
                  {
                    PrintError( error );
                    delete[] pCameras;
                    return -1;
                  }

               shutterType = GENERAL_EXTENDED_SHUTTER;
            }
            else
             {
               printf( "Frame rate and extended shutter are not supported... exiting\n" );
               delete[] pCameras;
               return -1;
             }

            // Set the shutter property of the camera

             Property shutter;
             shutter.type = SHUTTER;
             shutter.absControl = true;
             shutter.onePush = false;
             shutter.autoManualMode = false;
             shutter.onOff = true;
             shutter.absValue = shutt;

             error = pCameras[i].SetProperty( &shutter );
             if (error != PGRERROR_OK)
               {
                 PrintError( error );
                 delete[] pCameras;
                 return -1;
               }

             printf( "Shutter time set to %.2fms\n", shutt );

            // Start streaming on camera
            std::stringstream ss;
            ss << currentDateTime2();
            std::string st = ss.str();
            char pippo[st.length()];
            sprintf(pippo, "%s" , st.c_str() );
            printf( "Start time %s \n", pippo );


            error = pCameras[i].StartCapture();
            if (error != PGRERROR_OK)
              {
                PrintError(error);
                delete[] pCameras;
                return -1;
              }
          }

    
    int imageCnt=0; 
   for ( ; ;  ) //  start an indefinite grab images loop
     {

    	 for (unsigned int i = 0; i < numCameras; i++)
    	    {
    	      Image image;
    	      error = pCameras[i].RetrieveBuffer(&image);
    	      if (error != PGRERROR_OK)
    	        {
    	          PrintError(error);
    	          delete[] pCameras;
    	          return -1;
    	        }
    	      // Display the timestamps of the images grabbed for each camera
    	         TimeStamp timestamp = image.GetTimeStamp();
    	         //cout << "Camera " << i << " - Frame " << imageCnt << " - TimeStamp [" << timestamp.cycleSeconds << " " << timestamp.cycleCount << "]"<< endl;

    	         // Save the file

    	      unsigned int res =	 0; // for initialization purposes

    	      Property shutter;
    	      shutter.type = SHUTTER;
    	      error = pCameras[i].GetProperty( &shutter );
    	      if (error != PGRERROR_OK)
    	        {
    	          PrintError( error );
    	          return -1;
    	        }


    	      error = pCameras[i].GetCameraInfo(&camInfo);
    	      if (error != PGRERROR_OK)
    	        {
    	          PrintError(error);
    	          delete[] pCameras;
    	          return -1;
    	        }

    	       if (camInfo.serialNumber == 15568736)
    	         {
    	           error = pCameras[i].ReadRegister(0x82C, &ulValue); // read the temperature register for the NIR camera only
    	           unsigned r  = createMask(20, 31); // extract the bits of interest
   	               res = r & ulValue;  // here you have already the Kelvin temperature * 10
    	           if (error != PGRERROR_OK)
    	              {
    	                PrintError( error );
    	                delete[] pCameras;
    	                return -1;
    	              }
   	              }
    	        PixelFormat pixFormat;
    	        unsigned int rows, cols, stride;
    	        image.GetDimensions( &rows, &cols, &stride, &pixFormat );

    	       // Create a unique filename

    	       std::string str;     //temporary string to hold the filename
    	       int lengthOfString1; //hold the number of characters in the string
    	       std::stringstream sstm;
    	       std::string head;
    	       if (camInfo.serialNumber == 15568736)
    	         {
    	          head = "NIR";
    	         }
    	       else
    	          {
    	    	   head = "VIS";
    	          }

    	       sstm  << currentDateTime2();
    	       str = sstm.str();
    	       lengthOfString1=str.length();

    	       int lenghtsum = lengthOfString1 + 4  + 4;
    	       char filename[lenghtsum];
    	       sprintf(filename,"%s-%s.raw" , head.c_str(), str.c_str() );
    	       //     cout << filename << endl; // uncomment for testing purposes

    	       unsigned int iImageSize = image.GetDataSize();
    	       printf( "Grabbed image %s \n", filename );
//    	                 printf( "Frame rate is %3.1f fps\n", frmRate.absValue );
    	       printf( "Shutter is %3.1f ms\n", shutter.absValue );
    	       /*
             if (camInfo.serialNumber == 15568736)
    	         {
    	           cout << "Temperature is " << res / 10.0 << " K/ " << res / 10.0	- 273.15 << " Celsius" << endl;
    	         }
    	       cout << "Raw Image Dimensions: " << rows  << " x " << cols << " Image Stride: " << stride << endl;
    	       cout << "Image Size: " << iImageSize << endl;
            */
    	      // Save the image. If a file format is not passed in, then the file
    	     // extension is parsed to attempt to determine the file format.
    	       error = image.Save( filename );
    	       if (error != PGRERROR_OK)
    	         {
    	           PrintError( error );
    	           delete[] pCameras;
    	           return -1;
    	         }


      }
    	 imageCnt++;
     }

    std::stringstream ss1;
    ss1 << currentDateTime2();
    std::string st1= ss1.str();
    char pippo1[st1.length()];
    sprintf(pippo1 , "%s" , st1.c_str() );
    printf( "End time time %s \n", pippo1 );

    // disconnect the camera

    for (unsigned int  i= 0; i < numCameras; i++)
        {
          pCameras[i].StopCapture();
          pCameras[i].Disconnect();
        }

        delete[] pCameras;

    return 0;
}