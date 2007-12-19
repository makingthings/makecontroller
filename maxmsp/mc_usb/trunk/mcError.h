/*********************************************************************************

 Copyright 2006-2007 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#ifndef MCERROR_H
#define MCERROR_H

typedef enum 
{
  MC_OK                         = 0,   // All OK
  MC_NOTHING_AVAILABLE          = -1,  // No Usb data is available
  MC_ALREADY_OPEN               = -2,  // Port is already open
  MC_NOT_OPEN                   = -3,  // Port is not open
  MC_ERROR_CLOSE                = -4,  // There was an error closing
  MC_GOT_CHAR                   = -5,  // Successfully read a character from the USB port
  MC_IO_ERROR                   = -6,  // Usb error
  MC_UNKNOWN_ERROR              = -7,	 // Unknown error
  MC_PACKET_LENGTH_0            = -8,
  MC_ERROR_CREATING_BUNDLE      = -9,
  MC_ERROR_SENDING_TEXT_MESSAGE = -10
} mcError;

#endif


