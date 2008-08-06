/*********************************************************************************

 Copyright 2006-2008 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

/*
	SPI.h

  MakingThings
*/

#ifndef SPI_H
#define SPI_H

int  Spi_SetActive( void );

int  Spi_Start( int channel );
int  Spi_Stop( int channel );

int  Spi_Configure( int channel, int bits, int clockDivider, int delayBeforeSPCK, int delayBetweenTransfers );

int  Spi_Lock( void );
void Spi_Unlock( void );

int  Spi_ReadWriteBlock( int channel, unsigned char* buffer, int count );

#endif
