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
	servo_internal.h

  MakingThings
*/

#ifndef SERVO_INTERNAL_H
#define SERVO_INTERNAL_H

#include "AT91SAM7X256.h"

#define SERVO_COUNT 4

typedef struct ServoControlS
{
  int users;
  int speed;
  int positionRequested;
  int position;
  int pin;
  AT91S_PIO* pIoBase;
} ServoControl;

typedef struct ServoS
{
  int users;
  int gap;
  int index;
  int state;
  ServoControl control[ SERVO_COUNT ];
} Servo_;

#endif
