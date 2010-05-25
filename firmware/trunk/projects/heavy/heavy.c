
#include "core.h"
#include "appled.h"

//const OscNode oscSystems = {
//
//};

#define SZ 256
char buf[SZ];

Timer t;
int tester;

void timerHandler(void)
{
  tester++;
  timerGo(&t, 500, timerHandler);
}

void setup()
{
//  int i;
  // setup all the systems we want to use
  appledEnable();
//  digitaloutInit();
//  dipswitchInit();

  usbserialInit();
  tester = 0;
  timerGo(&t, 500, timerHandler);
//  networkInit();
//  char c = serialGet(0, 0);
}

void loop()
{
  ledSetValue(ON);
  sleep(10);
  ledSetValue(OFF);
  sleep(990);
  int rxed = usbserialRead(buf, sizeof(buf), -1);
  if (rxed > 0) {
    usbserialWrite(buf, rxed, 1000);
  }
//  usbserialWrite("hi\n", 3, 1000);
}

