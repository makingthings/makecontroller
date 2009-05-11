/*
	config.h - Select which features & hardware you're using.
  MakingThings
*/

#ifndef CONFIG_H
#define CONFIG_H

#include "controller.h"   // ...everybody uses the MAKE Controller Board
#include "error.h"

#define FIRMWARE_NAME          "Heavy svn"
#define FIRMWARE_MAJOR_VERSION 1
#define FIRMWARE_MINOR_VERSION 6
#define FIRMWARE_BUILD_NUMBER  2


#define CONTROLLER_HEAPSIZE 21000

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

//  The version of the MAKE Controller Board you're using.
#define CONTROLLER_VERSION  200    // valid options: 50, 90, 95, 100, 200

//  The version of the MAKE Application Board you're using.
#define APPBOARD_VERSION  100    // valid options: 50, 90, 95, 100, 200

#endif // CONFIG_H

