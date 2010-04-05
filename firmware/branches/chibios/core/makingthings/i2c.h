/*********************************************************************************

 Copyright 2006-2009 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#ifndef _I2C_H
#define _I2C_H

#include "ch.h"

// return values
#define I2C_OK              0x00
#define I2C_ERROR_NODEV     0x01
#define I2C_BUS_ERROR       0x07
#define I2C_TIMED_OUT       0x08
#define I2C_IN_PROGRESS     0x09

#ifdef __cplusplus
extern "C" {
#endif
void i2cInit(void);
void i2cSetBitrate(int rate);
void i2cAcquireBus(void);
void i2cReleaseBus(void);
int i2cRead(uint8_t deviceAddr, uint8_t *data, uint8_t length, uint16_t internalAddr, uint16_t intAddrLen);
int i2cWrite(uint8_t deviceAddr, const uint8_t *data, uint8_t length, uint16_t internalAddr, uint16_t intAddrLen);
#ifdef __cplusplus
}
#endif

#endif // _I2C_H




