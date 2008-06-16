/*********************************************************************************

 Copyright 2006 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

/** \file controller.c	
	Functions available to the Make Controller Board.
*/

/** @defgroup Controller
	Functions available to the Make Controller Board.
	
	The \b Controller module provides an API to devices that are all included on the Make Controller, no matter
	what interface electronics are used around it.  The \ref AppBoard module extends the Controller module to include
	the devices found on the Make Application Board.
*/

/**
 \mainpage Make Controller Kit - Firmware API
 
 \section overview Overview
	
	The Make Controller Board is a powerful, flexible, and easy to use microcontroller, based on the Atmel ARM7 SAM7X256.
	It is designed to interface easily to additional circuitry in order to connect with the real world.
	
	The Make Controller Board can be plugged into into the Make Application Board which makes use of nearly all the 
	devices on the Controller. Alternatively, it can be plugged into a piece of hardware of your creation.

	To jump right into browsing the API, click on \b Modules above.
		
	\section soft_env Software Environment 
	The Make Controller Kit software environment is a fusion of four main bodies of code:
	- User code - the applications that you, the user, create.
	- MakingThings library - code that MakingThings has contributed to make it easier to work with the Make Controller Kit.
	- FreeRTOS - an open source Real Time Operating System for the SAM7X.  See http://www.freertos.org
	- lwIP - a complete and open source TCIP/IP code library.  See http://www.sics.se/~adam/lwip/

  To ensure that the software environment for the Controller is consistent, regardless of the interface hardware,
	the code for the Controller Board has been kept separate from the code for the Application Board to make this as easy
	as possible:
	- The \ref Controller module provides the core routines required for the Make Controller Board.
	- The \ref AppBoard module provides routines that work with the Make Application Board, and are often based on or use
	code from the ControllerBoard module.
	- The \ref OSC module provides a communication interface to the Make Controller Kit from other computers and devices.

	Software for the Make Controller Kit is open source, and thus subject to the licensing of each module of code.  Be sure to acquaint 
	yourself with the licenses for the main bodies of code in the Make Controller firmware codebase:
	- MakingThings: http://www.apache.org/licenses/LICENSE-2.0.html
	- FreeRTOS: http://www.freertos.org/a00114.html
	- lwIP: http://www.xfree86.org/3.3.6/COPYRIGHT2.html#5

	\section Tools
	The firmware for the Make Controller Kit can be built using the freely available ARM-flavored version of the GCC compiler, \b arm-elf-gcc.
	Many toolchains are available - OS X and Windows versions can be found on the MakingThings downloads page: http://www.makingthings.com/resources/downloads
	Other good options include:
	- GNU-ARM: http://www.gnuarm.com
	- WinARM (Windows-only): http://www.siwawi.arubi.uni-kl.de/avr_projects/arm_projects/#winarm
	- YAGARTO (Windows-only): http://www.yagarto.de

	To upload new firmware to your Make Controller, you have a few options:
	- Use \b mchelper, Make Controller Helper, an app from MakingThings: http://www.makingthings.com/resources/downloads
	- Use \b sam7utils, a command line app: http://oss.tekno.us/sam7utils
	- Use a JTAG device.

	Rowley also provides a very good IDE for ARM7 projects called \b CrossWorks, which supports in-circuit debugging.  There are single user
	licenses available.  Check http://www.rowley.co.uk/arm/index.htm

	\section Community
	As you begin to create projects with the Make Controller, please consider contributing any potentially helpful material you generate
	or come across.  This could be anything from source code, to schematics, to instructions on how to interface with a particular device.

	The best places to get in touch are:
	- MakingThings forums: http://www.makingthings.com/forum
	- MakingThings bug tracker & development wiki: http://dev.makingthings.com
	- MakingThings IRC channel: \b makingthings on \b irc.freenode.net

	We look forward to hearing about your projects!
*/

/*---------------------------------------------------------------------------------------------------------------*/


#include "controller.h"
#include "pwm.h"
#include "adc.h"
#include "spi.h"
#include "USB-CDC.h"

/**	
	Controller Init.
	Initialize the Make Processor Board to work with the Make Application Board.
*/
void Controller_Start()
{

}

