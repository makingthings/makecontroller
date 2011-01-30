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

#include "core.h"

int main(int argc, char **argv)
{
  UNUSED(argc);
  UNUSED(argv);
  
  halInit();
  // peripheral inits - these are here so they're conveniently already
  // done for common usage, but can be removed by conditionalization
  #ifndef NO_SPI_INIT
  spiInit();
  #endif

  #ifndef NO_EEPROM_INIT
  eepromInit();
  #endif

  #ifndef NO_PWM_INIT
  pwmInit();
  #endif

  #ifndef NO_SERIAL_INIT
  sdInit();
  #endif

  chSysInit(); // ChibiOS/RT initialization.

  // would rather not put this below chSysInit() but it relies on the RTOS being setup
  #ifndef NO_AIN_INIT
  analoginInit();
  #endif

  setup();

  while (true) {
    loop();
  }

  return 0;
}
