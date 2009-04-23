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

#ifndef SERVO_H
#define SERVO_H

#include "io.h"
#include "fasttimer.h"

/**
  Speed and position control for up to 4 standard servo motors.
  Standard servos have a range of motion of approximately 180 degrees, although this varies from motor to motor.
  Be sure to plug in the connector with the correct orientation with regard to the GND/5V signals on the board.
  
  \section Usage
  To get started controlling Servos, create a Servo object, optionally set the speed, and then
  start moving it around using setPosition() as desired.
  \code
  Servo srv(1); // create a servo object controlling servo 1
  
  // inside a task...make the servo rock back and forth
  while(true) // forever
  {
    srv.setPosition(1023); // go all the way to one side
    while(srv.position() < 1023) // wait till we get there
      Task::sleep(10);
    srv.setPosition(0); // now go back the other way
    while(srv.position() > 0)
      Task::sleep(10);
  }
  \endcode
  
  \section range Range of Motion
  Because not all servo motors are created equal, and not all of them can be safely driven across all 180 degrees,
  the default (safe) range of motion is from values 0 to 1023.  The full range of motion is from -512 to 1536, but use this
  extra range cautiously if you don't know how your motor can handle it.  A little gentle experimentation should do the trick.
  
  You can also specify the speed with which the motors will respond to new position commands - a high
  value will result in an immediate response, while a lower value can offer some smoothing when appropriate.
  
  See the servo section in the <a href="http://www.makingthings.com/documentation/tutorial/application-board-overview/servos">
  Application Board overview</a> for more detailed info.
  \ingroup io
*/
class Servo
{
  public:
    Servo(int index);
    ~Servo();

    int position();
    bool setPosition(int position);

    int speed();
    bool setSpeed(int speed);

  protected:
    int _index;
    
    typedef struct
    {
      short refcount;
      int speed;
      int destination;
      int position;
      Io* io;
    } ServoInternal; // internal structure to represent each servo

    typedef struct
    {
      FastTimer fastTimer;
      bool state;
      int gap;
      short index;
      int activeDevices;
    } Base;

    static ServoInternal* servos[];
    static Base servoBase;

    int getIo( int index );
    void baseInit();
    void baseDeinit();
    friend void Servo_IRQCallback( int id );
};

/* OSC Interface */
const char* ServoOsc_GetName( void );
int ServoOsc_ReceiveMessage( int channel, char* message, int length );

#endif
