#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcitx/module.h>
#include <fcitx/ime.h>
#include <fcitx/instance.h>
#include <fcitx/candidate.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx-config/xdg.h>
#include <fcitx-config/hotkey.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/utils.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/instance.h>
#include <fcitx/context.h>
#include <fcitx/keys.h>
#include <fcitx/ui.h>
#include <libintl.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <X11/Xlib.h>

#include "pen.h"
#include "config.h"
#include "driver.h"

//drivers
#include "lxbi.h"

typedef struct {
	Display* dpy;
	GC gc;
	XGCValues gcv;
} TabletX;

typedef struct {
	FcitxTabletDriver* drv;
	void* userdata;
	char* packet;
	int fd;
} TabletDriver;

typedef struct {
	pt_t* buffer;
	pt_t* ptr;
	unsigned n;
} TabletStrokes;

typedef struct _FcitxTabletPen {
	FcitxTabletConfig conf;
	TabletX x;
	TabletDriver driver;
	TabletStrokes strokes;
	FcitxInstance* fcitx;
} FcitxTabletPen;


pt_t** GetStrokeBufferLocation(FcitxTabletPen* tablet, FcitxModuleFunctionArg args) {
	return &tablet->strokes.buffer;
}


void* FcitxTabletCreate(FcitxInstance* instance) {
	FcitxTabletPen* tablet = fcitx_utils_new(FcitxTabletPen);
	FcitxTabletLoadConfig(&tablet->conf);
	// todo select driver from config

	{ // Initialise the driver
		TabletDriver* d = &tablet->driver;
		d->drv = &lxbi;
		d->userdata = d->drv->Create();
		d->packet = (char*) malloc(d->drv->PacketSize(d->userdata));

		d->fd = open(tablet->conf.devicePath, O_RDONLY);
		if(d->fd < 0) {
			FcitxLog(ERROR, "Unable to open device %s", tablet->conf.devicePath);
			return NULL;
		}
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

	}

	{ // Initialise the stroke buffer
		TabletStrokes* s = &tablet->strokes;
		s->n = 2048;
		s->buffer = (pt_t*) malloc(sizeof(pt_t) * s->n);
		s->ptr = s->buffer;
	}

	//XGrabButton(TabletModule->dpy, 1, AnyModifier, DefaultRootWindow(TabletModule->dpy), True, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);

	FcitxLog(WARNING, "TabletModule init ok");
	FcitxLog(WARNING, "Openining pipe");
//	TabletModule->ic = FcitxInstanceCreateIC(instance, 1, NULL);

	tablet->fcitx = instance;

	//return instance;
	FcitxAddon* tablet_addon = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance), FCITX_TABLET_NAME);
	AddFunction(tablet_addon, GetStrokeBufferLocation);

	return tablet;
}

void FcitxTabletSetFd(void* arg) {
	FcitxTabletPen* tablet = (FcitxTabletPen*) arg;
	FD_SET(tablet->driver.fd, FcitxInstanceGetReadFDSet(tablet->fcitx));
	if(FcitxInstanceGetMaxFD(tablet->fcitx) < tablet->driver.fd)
		FcitxInstanceSetMaxFD(tablet->fcitx, tablet->driver.fd);
}


void FcitxTabletProcess(void* arg) {
	FcitxTabletPen* tablet = (FcitxTabletPen*) arg;
	TabletDriver* d = &tablet->driver;
	if(FD_ISSET(d->fd, FcitxInstanceGetReadFDSet(tablet->fcitx))) {

		{ // first read a packet from the raw device
			int n = 0;
			const int pktsize = d->drv->PacketSize(d->userdata);
			do {
				n += read(d->fd, &d->packet[n], pktsize - n);
			} while(n < pktsize);
		}
		//FcitxLog(WARNING, "Read %d bytes", n);

		{ // then send it to the driver to convert into events
			FcitxTabletDriverEvent e;
			pt_t pt;
			while((e = d->drv->GetEvent(d->userdata, d->packet, &pt)) != EV_NONE) {
				switch(e) {
				case EV_PENDOWN:
					break; // nothing
				case EV_PENUP: // run recognition
					FcitxInstanceProcessKey(tablet->fcitx, FCITX_PRESS_KEY, 0, FcitxKey_VoidSymbol, 0);
					break;
				case EV_POINT: {
					TabletStrokes* s = &tablet->strokes;
					*s->ptr++ = pt;
					if(s->ptr > &s->buffer[s->n]) { // if we overflow the buffer
						int newsize = s->n + 1024;
						s->buffer = (pt_t*) realloc(s->buffer, newsize);
						s->ptr = &s->buffer[s->n+1];
						s->n = newsize;
					}
				}
					break;
				default:
					FcitxLog(ERROR, "Driver returned unknown event: %d", e);
					break;
				}
			}
		}

		{ // draw the stroke on the screen
			TabletX* x = &tablet->x;
			x->gcv.line_width = 15;
			XChangeGC(x->dpy, x->gc, GCLineWidth, &x->gcv);
			XSetForeground(x->dpy, x->gc, BlackPixel(x->dpy, DefaultScreen(x->dpy)));
			XDrawLine(x->dpy, DefaultRootWindow(x->dpy), x->gc, 100, 100, 300, 300);
			XSync(x->dpy, 0);
		}

		FD_CLR(d->fd, FcitxInstanceGetReadFDSet(tablet->fcitx));
	}
}

void FcitxTabletDestroy(void* arg) {
	FcitxLog(WARNING, "FcitxTabletDestroy");
	FcitxTabletPen* tablet = (FcitxTabletPen*) arg;
	XFreeGC(tablet->x.dpy, tablet->x.gc);
	XCloseDisplay(tablet->x.dpy);
	tablet->driver.drv->Destroy(tablet->driver.userdata);
	free(tablet->driver.packet);
	free(tablet->strokes.buffer);
}




FCITX_EXPORT_API
FcitxModule module = {
	FcitxTabletCreate,
	FcitxTabletSetFd,
	FcitxTabletProcess,
	FcitxTabletDestroy,
	NULL
};
