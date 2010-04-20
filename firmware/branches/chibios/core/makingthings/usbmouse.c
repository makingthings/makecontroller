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

#include "usbmouse.h"
#include "HIDDMouseDriver.h"
#include "USBConfigurationDescriptor.h"
#include "USBD.h"

/**
  \defgroup usbmouse USB Mouse (HID)
  Allow the Make Controller to behave like a mouse for your laptop or desktop.
  You can send right and left click events, as well as send mouse movement events.

  \section Usage
  First, initialize the USB system by calling usbmouseInit().  Then, call usbmouseUpdate()
  to send mouse click and movement events.

  \section Building
  You'll need to adjust your Makefile to build as a USB mouse.  In the \b CSRC section,
  add ${USBHIDCORE} and ${USBHIDMOUSE}, and in the \b INCDIR section, add ${USBHIDCOREINC} and
  ${USBHIDMOUSEINC}, and add ${MT}/usbmouse.c.
  \ingroup interfacing
  @{
*/

/**
  Initialize the Make Controller as a USB mouse.
*/
void usbmouseInit() {
  HIDDMouseDriver_Initialize();
  USBD_Connect();
}

bool usbmouseIsActive(void) {
  return USBD_GetState() == USBD_STATE_CONFIGURED;
}

/**
 * Send new mouse activity to the host computer.
 * Send left-click or right-click mouse events and/or update the position of
 * the mouse.
 *
 * Specify clicks on what would be the right and left buttons of the mouse via
 * the appropriate MouseClickAction - DOWN, UP, or NONE.
 *
 * The position of the mouse is updated from its current location -
 * if you specify a changeX of 10, the mouse will move 10 pixels to the right,
 * -10 will take you 10 pixels to the left.  A changeY of 10 will take you 10
 * pixels downwards and -10 will bring you 10 pixels up.
 *
 * @param left A left-click action - DOWN, UP, or NONE.
 * @param right A right-click action - DOWN, UP, or NONE.
 * @param changeX A change in position (specified in pixels) on the x-axis.
 * @param changeY A change in position (specified in pixels) on the y-axis.
 * @return True if the message was successfully sent, otherwise false.
 */
bool usbmouseUpdate(MouseClickAction left, MouseClickAction right, int changeX, int changeY) {

  if (USBD_GetState() != USBD_STATE_CONFIGURED)
    return false;
  unsigned char buttons = 0;

  if (left == DOWN)
    buttons |= HIDDMouse_LEFT_BUTTON;
  else if (left == UP)
    buttons &= ~HIDDMouse_LEFT_BUTTON;

  if (right == DOWN)
    buttons |= HIDDMouse_RIGHT_BUTTON;
  else if (right == UP)
    buttons &= ~HIDDMouse_RIGHT_BUTTON;

  return HIDDMouseDriver_ChangePoints(buttons, (signed char)changeX, (signed char)changeY) == USBD_STATUS_SUCCESS;
}

/** @} */



