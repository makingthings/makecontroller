/*********************************************************************************

 Copyright 2006-2010 MakingThings

 Licensed under the Apache License,
 Version 2.0 (the "License"); you may not use this file except in compliance
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#ifndef MT_SERIAL_H
#define MT_SERIAL_H

#include "types.h"
#include "ch.h"
#include "hal.h"

typedef SerialDriver* Serial;

#define Serial0    (&SD1) /**< symbol for serial port 0 */
#define Serial1    (&SD2) /**< symbol for serial port 1 */
#define SerialDbg  (&SD3) /**< symbol for the debug serial port */

#ifdef __cplusplus
extern "C" {
#endif
void serialEnable(Serial port, int baud);
void serialEnableAll(Serial port, int baud, int parity, int charbits, int stopbits, bool handshake);
void serialDisable(Serial port);
int  serialAvailable(Serial port);
int  serialRead(Serial port, char* buf, int len, int timeout);
char serialGet(Serial port, int timeout);
int  serialWrite(Serial port, char const* buf, int len, int timeout);
int  serialPut(Serial port, char c, int timeout);
#ifdef __cplusplus
}
#endif
#endif // MT_SERIAL_H
