/*
	config.h - Select which features & hardware you're using.
  MakingThings
*/

#ifndef CONFIG_H
#define CONFIG_H

#include "controller.h"   // ...everybody uses the MAKE Controller Board
#include "error.h"

#define FIRMWARE_NAME          "Solo-svn"
#define FIRMWARE_MAJOR_VERSION 1
#define FIRMWARE_MINOR_VERSION 6
#define FIRMWARE_BUILD_NUMBER  0


#define CONTROLLER_HEAPSIZE 20000

//----------------------------------------------------------------
//  Comment out the systems that you don't want to include in your build.
//----------------------------------------------------------------
#define MAKE_CTRL_USB     // enable the USB system
#define MAKE_CTRL_NETWORK // enable the Ethernet system
#define OSC               // enable the OSC system

// ---------------------------------------------------------------
// Network options
// ---------------------------------------------------------------
#define NETWORK_MEM_POOL         2000 // the network system's memory pool
#define NETWORK_UDP_CONNS        4    // the number of possible DatagramSocket instances
#define NETWORK_TCP_CONNS        4    // the number of possible Socket instances
#define NETWORK_TCP_LISTEN_CONNS 2    // the number of possible ServerSocket instances

//----------------------------------------------------------------
//  Uncomment the revision of the MAKE Controller Board you're using.
//----------------------------------------------------------------
//#define CONTROLLER_VERSION  50    // Atmel SAM7X eval. board
//#define CONTROLLER_VERSION  90     
//#define CONTROLLER_VERSION  95       
#define CONTROLLER_VERSION  100        

//----------------------------------------------------------------
//  Uncomment the revision of the MAKE Application Board you're using.
//----------------------------------------------------------------
//#define APPBOARD_VERSION  50    // Atmel SAM7X eval. board
//#define APPBOARD_VERSION  90   
//#define APPBOARD_VERSION  95    
#define APPBOARD_VERSION  100   

#endif // CONFIG_H

