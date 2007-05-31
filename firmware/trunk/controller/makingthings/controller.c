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

// MakingThings - Make Controller Kit - 2006

/** \file controller.c	
	Library of functions for the Make Controller Board.
*/

/** @defgroup Controller
	A library of functions for the Make Controller Board.
	
*/

/**
 \mainpage MAKE Controller Kit - C API Reference
 
 \section overview Overview
	
	The MAKE Controller Board is a powerful, flexible, and easy to use microcontroller, based on the Atmel ARM7 SAM7X256.
	The range of applications it can perform  by itself is rather limited, however.  It is designed to interface easily to 
	additional circuitry in order to actually connect with the real world.
	
	The MAKE Controller Board can be plugged into into the MAKE Application Board which makes use of nearly all the 
	devices on the Controller or it can, alternatively, be plugged into a piece of hardware of your creation.
		
	To ensure that the software environment for the Controller is consistent, regardless of the interface hardware,
	the code for the Controller Board has been kept separate from the code for the Application Board to make this as easy
	as possible:
	- The <b>ControllerBoard</b> module provides the core routines required for the MAKE Controller Board.
	- The <b>AppBoard</b> module provides routines that work with the MAKE Application Board, and are often based on or use
	code from the ControllerBoard module.
	
	\subsection soft_env Software Environment 
	The Make Controller Kit software environment is a fusion of four main bodies of code:
	- User code - the applications that you, the user, create.
	- MakingThings library - code that MakingThings has contributed to make it easier to work with the MAKE Controller Kit.
	- FreeRTOS - an open source Real Time Operating System for the SAM7X.  See http://www.freetros.org
	- LwIP - a complete and open source TCIP/IP code library.  See http://www.sics.se/~adam/lwip/

  These bodies of code represent tens of 1000's of hours of work by dedicated people.  They don't always look alike.

 \section philosophy Philosophy

 The MAKE Controller Kit is a community project, designed to be rewarding to different groups:
	- Fabricators
	- Coders
	- Electrical Engineers
	- Students
	
	and everybody in between.  The idea is to get beginners to start off as simply as possible with the real thing,
	so they develop real skills, and provide a full-featured system & environment for more experienced developers.
 
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

