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
#include "board.h"
#include "hal.h"
#include "led.h"

int main(int argc, char **argv) {

  UNUSED(argc);
  UNUSED(argv);

  ledEnable();

  while (true) {
    chThdSleepMilliseconds(990);
    ledSetValue(ON);
    chThdSleepMilliseconds(10);
    ledSetValue(OFF);
  }

  return 0;
}
