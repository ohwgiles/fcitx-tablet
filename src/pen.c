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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <fcitx/module.h>
#include <fcitx-utils/log.h>
#include <fcitx/context.h>
#include <fcntl.h>
#include "pen.h"
#include "config.h"
#include "driver.h"
#include "ime.h"

// pen.c
// This file contains the fcitx event module. It's needed to add an fd to
// the main select() loop to catch events from the tablet device. It also
// contains routines for drawing on the X display and implements a timeout
// to instruct the IME to commit the most likely candidate


// Access the config from other modules
FcitxTabletPen* GetConfig(FcitxTabletPen* tablet, FcitxModuleFunctionArg args) {
	return tablet;
}

// Drivers, see driver.h
extern FcitxTabletDriver lxbi;

// Recognisers, see recog.h
extern FcitxTabletRecogniser recogfork;


void* FcitxTabletCreate(FcitxInstance* instance) {
	FcitxTabletPen* tablet = fcitx_utils_new(FcitxTabletPen);
	FcitxTabletLoadConfig(&tablet->conf);
	// TODO select driver from config, currently using lxbi

	{ // Initialise the driver
		TabletDriver* d = &tablet->driver;
		d->drv = &lxbi;
		d->userdata = d->drv->Create();
		d->packet = (char*) malloc(d->drv->packet_size);

	}

	{ // Initialise the X display
		TabletX* x = &tablet->x;
		if(!(x->dpy = XOpenDisplay(NULL)))  {
			FcitxLog(ERROR, "Unable to open X display");
			return NULL;
		}
		x->gcv.function = GXcopy;
		x->gcv.subwindow_mode = IncludeInferiors;
		x->gcv.line_width = 8;
		x->gcv.cap_style = CapRound;
		x->gcv.join_style = JoinRound;
		x->gc = XCreateGC(x->dpy, DefaultRootWindow(x->dpy), GCFunction | GCSubwindowMode | GCLineWidth | GCCapStyle | GCJoinStyle, &x->gcv);
		XSetForeground(x->dpy, x->gc, WhitePixel(x->dpy, DefaultScreen(x->dpy)));
	}

	{ // Initialise the stroke buffer
		TabletStrokes* s = &tablet->strokes;
		s->n = 2048;
		s->buffer = (pt_t*) malloc(sizeof(pt_t) * s->n);
		s->ptr = s->buffer;
	}

	{ // instantiate the recogniser
		// TODO select from config
		tablet->recog = &recogfork;
		tablet->recog_ud = recogfork.Create(&tablet->conf);
	}

	tablet->fcitx = instance;
	return tablet;
}

void FcitxTabletSetFd(void* arg) {
	FcitxTabletPen* tablet = (FcitxTabletPen*) arg;
	int fd = tablet->driver.drv->GetDescriptor(tablet->driver.userdata);
   if(fd > 0) {
      FD_SET(fd, FcitxInstanceGetReadFDSet(tablet->fcitx));
      if(FcitxInstanceGetMaxFD(tablet->fcitx) < fd)
         FcitxInstanceSetMaxFD(tablet->fcitx, fd);
   }
}

void PushCoordinate(TabletStrokes* s, pt_t newpt) {
	*s->ptr++ = newpt;
	if(s->ptr == &s->buffer[s->n]) { // if we overflow the buffer, increase it
		FcitxLog(WARNING, "Resizing stroke buffer");
		int newsize = s->n + 1024;
		pt_t* newbuf = (pt_t*) realloc(s->buffer, sizeof(pt_t)*newsize);
		if(newbuf == NULL)
			FcitxLog(ERROR, "Failed to allocate more stroke memory");
		s->buffer = newbuf;
		s->ptr = &s->buffer[s->n];
		s->n = newsize;
	}
}

void FcitxTabletProcess(void* arg) {
	FcitxTabletPen* tablet = (FcitxTabletPen*) arg;
	TabletDriver* d = &tablet->driver;
	int fd = tablet->driver.drv->GetDescriptor(tablet->driver.userdata);
	if(fd > 0 && FD_ISSET(fd, FcitxInstanceGetReadFDSet(tablet->fcitx))) {

		{ // first read a packet from the raw device
			int n = 0;
			const int pktsize = d->drv->packet_size;
			do {
				n += read(fd, &d->packet[n], pktsize - n);
			} while(n < pktsize);
		}

		boolean redraw = false;
		{ // then send it to the driver to convert into events
			FcitxTabletDriverEvent e;
			pt_t pt;
			while((e = d->drv->GetEvent(d->userdata, d->packet, &pt)) != EV_NONE) {
				switch(e) {
				case EV_PENDOWN:
					break; // nothing
				case EV_PENUP:
					{ pt_t p = PT_INVALID; PushCoordinate(&tablet->strokes, p); }
					FcitxInstanceProcessKey(tablet->fcitx, FCITX_PRESS_KEY, 0, FcitxKey_VoidSymbol, IME_RECOGNISE);
					break;
				case EV_POINT: {
					TabletX* x = &tablet->x;
					TabletStrokes* s = &tablet->strokes;
					pt.x = (float) pt.x * (float) DisplayWidth(x->dpy,0) / (float) d->drv->x_max;
					pt.y = (float) pt.y * (float) DisplayHeight(x->dpy,0) / (float) d->drv->y_max;
					if(s->ptr > s->buffer && PT_ISVALID(s->ptr[-1]) && PT_ISVALID(pt)) { //we have at least 2 valid new points
						XDrawLine(x->dpy, DefaultRootWindow(x->dpy), x->gc, s->ptr[-1].x, s->ptr[-1].y, pt.x, pt.y);
						redraw = true;
					}
					PushCoordinate(&tablet->strokes, pt);
				} break;
				default:
					FcitxLog(ERROR, "Driver returned unknown event: %d", e);
					break;
				}
			}
		}

		if(redraw) { // draw the stroke on the screen
			XSync(tablet->x.dpy, 0);
		}

		FD_CLR(fd, FcitxInstanceGetReadFDSet(tablet->fcitx));
	}
}

void FcitxTabletDestroy(void* arg) {
	FcitxTabletPen* tablet = (FcitxTabletPen*) arg;
	XFreeGC(tablet->x.dpy, tablet->x.gc);
	XCloseDisplay(tablet->x.dpy);
	tablet->driver.drv->Destroy(tablet->driver.userdata);
	free(tablet->driver.packet);
	free(tablet->strokes.buffer);
}

// Instantiate the event module
FCITX_EXPORT_API
FcitxModule module = {
	FcitxTabletCreate,
	FcitxTabletSetFd,
	FcitxTabletProcess,
	FcitxTabletDestroy,
	NULL
};
