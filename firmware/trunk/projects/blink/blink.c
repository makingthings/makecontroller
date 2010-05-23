

#include "core.h"

// this is called when the board turns on
void setup()
{

}

// loop here forever...just blink the Controller's LED
void loop()
{
  ledSetValue(ON);
  sleep(10);
  ledSetValue(OFF);
  sleep(990);
}

