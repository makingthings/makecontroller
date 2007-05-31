/*
	config.h

  Select which features & hardware you're using.

  MakingThings
*/

#ifndef CONFIG_H
#define CONFIG_H

//----------------------------------------------------------------
//  Comment out the systems that you don't want to include in your build.
// todo: make sure the code is organized so these are meaningful
//----------------------------------------------------------------
#define USB  // enable the USB system
#define NETWORK // enable the Ethernet system
#define OSC // enable the OSC system
#define FACTORY_TESTING

#include "controller.h"   // ...everybody uses the MAKE Controller Board
#include "appboard.h"     // ...if you're using the MAKE Application Board
#include "error.h"

#define FIRMWARE_VERSION_NUMBER 120
#define FIRMWARE_BUILD_NUMBER     0

#define CONTROLLER_HEAPSIZE 20650


//----------------------------------------------------------------
//  Uncomment the revision of the MAKE Controller Board you're using.
//----------------------------------------------------------------
//#define CONTROLLER_VERSION    50    // Atmel SAM7X eval. board
//#define CONTROLLER_VERSION    90     
//#define CONTROLLER_VERSION  95       
#define CONTROLLER_VERSION  100        

//----------------------------------------------------------------
//  Uncomment the revision of the MAKE Application Board you're using.
//----------------------------------------------------------------
//#define APPBOARD_VERSION    50    // Atmel SAM7X eval. board
//#define APPBOARD_VERSION    90   
//#define APPBOARD_VERSION  95    
#define APPBOARD_VERSION  100   

#endif // CONFIG_H

