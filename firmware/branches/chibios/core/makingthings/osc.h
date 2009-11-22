

#ifndef OSC_H
#define OSC_H

#include "config.h"
#include "types.h"

#ifdef MAKE_CTRL_USB
bool oscSetUsbListener(bool on);
#endif // MAKE_CTRL_USB

#endif // OSC_H

