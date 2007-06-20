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

#ifndef PACKETINTERFACE_H
#define PACKETINTERFACE_H

#include <QString>
#include "PacketReadyInterface.h"

class PacketInterface
{		
	public:
		enum Status { OK, ERROR_CANT_BIND, ERROR_CANT_SEND, ERROR_CANT_GET_ADDRESS, ERROR_NOT_OPEN };
	  	  
	  virtual Status open( ) = 0;
	  virtual Status close( ) = 0;
	  virtual int sendPacket( char* packet, int length ) = 0;
	  virtual bool isPacketWaiting( ) = 0;
	  virtual int receivePacket( char* buffer, int length ) = 0;
	  virtual char* location( ) = 0;
	  virtual QString getKey( ) = 0;
	  virtual void setPacketReadyInterface( PacketReadyInterface* packetReadyInterface) = 0;
	  virtual ~PacketInterface( ) {}
};

#endif
