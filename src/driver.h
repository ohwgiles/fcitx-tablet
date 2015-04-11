#ifndef FCITX_TABLET_DRIVER_H
#define FCITX_TABLET_DRIVER_H
/**************************************************************************
 *
 *  fcitx-tablet : graphics tablet input for fcitx input method framework
 *  Copyright 2012  Oliver Giles
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 **************************************************************************/

#include "point.h"

// Possible events emitted by a tablet driver. See FcitxTabletProcess in pen.c
typedef enum {
	EV_NONE,
	EV_PENUP,
	EV_PENDOWN,
	EV_POINT
} FcitxTabletDriverEvent;

// Interfaces implemented by a tablet driver
typedef struct _FcitxTabletDriver {
	void* (*Create)(const char* dev);
	// get the file descriptor of the device, to put into select()
	int (*GetDescriptor)(void*);
	unsigned packet_size; // the amount of bytes to read from the fd
	// get an event. The first argument is userdata, the second is the
	// raw data buffer from the tablet. If the event is EV_POINT,
	// this function should set the coordinates in the third argument
	FcitxTabletDriverEvent (*GetEvent)(void*, const char*, pt_t*);
	void (*Destroy)(void*);
} FcitxTabletDriver;

#endif
