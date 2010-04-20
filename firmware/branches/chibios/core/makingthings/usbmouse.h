

#ifndef USB_MOUSE_H
#define USB_MOUSE_H

#include "config.h"
#include "types.h"

typedef enum MouseClickAction_t {
  NONE,
  DOWN,
  UP
} MouseClickAction;

#ifdef __cplusplus
extern "C" {
#endif
void usbmouseInit(void);
bool usbmouseIsActive(void);
bool usbmouseUpdate(MouseClickAction left, MouseClickAction right, int changeX, int changeY);
#ifdef __cplusplus
}
#endif

#endif // USB_MOUSE_H


