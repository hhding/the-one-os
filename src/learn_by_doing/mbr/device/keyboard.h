#ifndef __DEVICE_KEYBOARD_H
#define __DEVICE_KEYBOARD_H
#include "stdint.h"
#include "ioqueue.h"

void keyboard_init(void);
extern struct ioqueue keyboard_ioq;
#endif
