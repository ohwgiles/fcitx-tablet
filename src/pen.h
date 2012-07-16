#ifndef FCITX_TABLET_PEN_H
#define FCITX_TABLET_PEN_H
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

#include <fcitx/instance.h>
#include <X11/Xlib.h>
#include "config.h"
#include "driver.h"
#include "point.h"
#include "recog.h"

#define FCITX_TABLET_NAME "fcitx-tablet-pen"

// Persistent storage for data relating to X drawing
typedef struct {
	Display* dpy;
	GC gc;
	XGCValues gcv;
} TabletX;

// Persistent storage for data relating to the tablet driver
typedef struct {
	FcitxTabletDriver* drv;
	void* userdata; // the driver's persistent data
	char* packet; // buffer for a packet from the driver
	int fd;
} TabletDriver;

// Persistent storage for the actual strokes. The points should be
// scaled to screen resolution boundaries before being stored here
typedef struct {
	pt_t* buffer; // start of buffer
	pt_t* ptr; // moving pointer
	unsigned n;
} TabletStrokes;

// Wrapper struct to hold all the above
typedef struct {
	FcitxTabletConfig conf;
	TabletX x;
	TabletDriver driver;
	FcitxTabletRecogniser* recog;
	void* recog_ud;
	TabletStrokes strokes;
	FcitxInstance* fcitx;
} FcitxTabletPen;

#endif
