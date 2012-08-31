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
#include <fcitx-utils/log.h>
#include <fcitx-config/hotkey.h>
#include <fcitx/keys.h>
#include <fcitx/candidate.h>
#include <fcitx/context.h>
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
#include "config.h"
#include "driver.h"
#include <X11/Xatom.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shapeconst.h>
#include <fcitx/module.h>
#include "point.h"
#include "engine.h"
#include "config.h"
#include <fcitx/instance.h>
#include <X11/Xlib.h>
#include "config.h"
#include "driver.h"
#include "point.h"
#include "engine.h"

#define FCITX_TABLET_NAME "fcitx-tablet-pen"

// Possible actions for the input method, triggered by tablet events
// such as timeouts and penups. See FcitxTabletProcess
typedef enum {
	IME_RECOGNISE,
	IME_COMMIT
} ImeAction;

// Persistent storage for data relating to X drawing
typedef struct {
	Display* dpy;
	Window win;
	int w,h;
	GC gc;
	XGCValues gcv;
} TabletX;

// Persistent storage for data relating to the tablet driver
typedef struct {
	FcitxTabletDriver* drv;
	void* userdata; // the driver's persistent data
	char* packet; // buffer for a packet from the driver
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
	TabletEngine* engine;
	void* engine_ud;
	TabletStrokes strokes;
	FcitxInstance* fcitx;
} FcitxTabletPen;

// Drivers, see driver.h
extern FcitxTabletDriver lxbi;

// Engines, see engine.h
extern TabletEngine engFork;
extern TabletEngine engZinnia;

boolean CharacterInProgress(TabletStrokes* strokes) {
	// a character drawing is in process if the current position pointer
	// is not at the beginning of the stroke buffer
	return strokes->ptr != strokes->buffer;
}

void ClearCharacter(FcitxTabletPen* d) {
	d->strokes.ptr = d->strokes.buffer;
	XClearWindow(d->x.dpy, d->x.win);
	XFlush(d->x.dpy);
}

INPUT_RETURN_VALUE FcitxTabletGetCandWords(void* arg);
INPUT_RETURN_VALUE FcitxTabletDoInput(void* arg, FcitxKeySym sym, unsigned int action) {
	// The event module uses VoidSymbol as a trigger, this means other input methods
	// will ignore it, actual actions will be passed in the action variable,
	// see ImeAction in ime.h
	FcitxTabletPen* tablet = (FcitxTabletPen*) arg;
	if(sym == FcitxKey_VoidSymbol) {
		switch(action) {
		case IME_RECOGNISE:
			tablet->engine->Process(tablet->engine_ud, tablet->strokes.buffer, (tablet->strokes.ptr - tablet->strokes.buffer));
			// call into recognition library, update candidate lists
			return IRV_DISPLAY_CANDWORDS;
			break;
		case IME_COMMIT:
			return IRV_COMMIT_STRING;
			break;
		default:
			FcitxLog(ERROR, "IME asked to perform unknown action");
		}
		return IRV_TO_PROCESS;
	}

	if(FcitxHotkeyIsHotKey(sym, action, FCITX_BACKSPACE)) {
		if(CharacterInProgress(&tablet->strokes)) {
			ClearCharacter(tablet);
			return IRV_CLEAN;
		} else
			return IRV_TO_PROCESS;
	} else if(FcitxHotkeyIsHotKey(sym, action, FCITX_SPACE) || FcitxHotkeyIsHotKey(sym, action, FCITX_ENTER)) {
		if(CharacterInProgress(&tablet->strokes)) {
			char s[5];
			char* candidates = tablet->engine->GetCandidates(tablet->engine_ud);
			int l = mblen(candidates, 10);
			memcpy(s, candidates, l);
			s[l] = '\0';
			FcitxInputContext* ic = FcitxInstanceGetCurrentIC(tablet->fcitx);
			FcitxInstanceCommitString(tablet->fcitx, ic, s);
			return IRV_CLEAN;
		} else
			return IRV_TO_PROCESS;
	}
	return IRV_TO_PROCESS;
}


// TODO maybe not needed
boolean FcitxTabletInit(void* arg) {
	return true;
}

FcitxIMClass ime;
void FcitxTabletReset(void* arg) {
	FcitxTabletPen* d = (FcitxTabletPen*) arg;
	// reset stroke buffer
	ClearCharacter(d);
	// get rid of the strokes covering the screen
	// TODO programmatically
	//system("xrefresh");
	FcitxLog(WARNING, "reset");

	XUnmapWindow(d->x.dpy, d->x.win);
	 XFlush(d->x.dpy);
}


INPUT_RETURN_VALUE FcitxTabletGetCandWord(void* arg, FcitxCandidateWord* c);

INPUT_RETURN_VALUE FcitxTabletGetCandWords(void* arg) {
	FcitxLog(WARNING, "FcitxTabletGetCandWords");
	FcitxTabletPen* d = (FcitxTabletPen*) arg;

	 FcitxInputState *input = FcitxInstanceGetInputState(d->fcitx);
	 FcitxInstanceCleanInputWindow(d->fcitx);
	 char* c = d->engine->GetCandidates(d->engine_ud);
	 int len = strlen(c);
	 int i = 0;
	 do {
		int n = mblen(&c[i], len);
		if(n <= 0) break;
		FcitxCandidateWord cw;
		cw.callback = FcitxTabletGetCandWord;
		cw.strExtra = NULL;
		cw.priv = NULL;
		cw.owner = d;
		cw.wordType = MSG_OTHER;
		cw.strWord = (char*) malloc(n+1);
		memcpy(cw.strWord, &c[i], n);
		cw.strWord[n] = 0;
		FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &cw);
		i += n;
	 } while(1);
	return IRV_DISPLAY_CANDWORDS;
}

INPUT_RETURN_VALUE FcitxTabletGetCandWord(void* arg, FcitxCandidateWord* candWord) {
	FcitxLog(WARNING, "FcitxTabletGetCandWord");
	FcitxTabletPen* d = (FcitxTabletPen*) candWord->owner;
	FcitxInputState *input = FcitxInstanceGetInputState(d->fcitx);
	strcpy(FcitxInputStateGetOutputString(input), candWord->strWord);
	return IRV_COMMIT_STRING;
}

void FcitxTabletImeDestroy(void* arg) {
	FcitxLog(WARNING, "destroy");
}

void* FcitxTabletImeCreate(FcitxInstance* instance) {
	FcitxTabletPen* ud = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance), FCITX_TABLET_NAME)->addonInstance;

	FcitxInstanceRegisterIM(
				instance,
				ud, //userdata
				"Tablet",
				"Tablet",
				"tablet",
				FcitxTabletInit,
				FcitxTabletReset,
				FcitxTabletDoInput,
				FcitxTabletGetCandWords,
				NULL,
				NULL,
				NULL,
				NULL,
				1,
				"zh_CN"
				);


	return ud;
}


FCITX_EXPORT_API FcitxIMClass ime = {
	FcitxTabletImeCreate,
	FcitxTabletImeDestroy
};


// Access the config from other modules
FcitxTabletPen* GetConfig(FcitxTabletPen* tablet, FcitxModuleFunctionArg args) {
	return tablet;
}

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
		//int blk = BlackPixel(x->dpy, DefaultScreen(x->dpy));
		x->w = 250;
		x->h = 150;
		XSetWindowAttributes attrs;
		attrs.override_redirect = True;
		attrs.background_pixel = WhitePixel(x->dpy, DefaultScreen(x->dpy));
		x->win = XCreateWindow(x->dpy, DefaultRootWindow(x->dpy), 0, 0, x->w, x->h, 0, CopyFromParent, InputOutput, CopyFromParent, CWBackPixel| CWOverrideRedirect, &attrs);
		x->gcv.function = GXcopy;
		x->gcv.subwindow_mode = IncludeInferiors;
		x->gcv.line_width = 4;
		x->gcv.cap_style = CapRound;
		x->gcv.join_style = JoinRound;
		x->gc = XCreateGC(x->dpy, x->win, GCFunction | GCSubwindowMode | GCLineWidth | GCCapStyle | GCJoinStyle, &x->gcv);
		XSetForeground(x->dpy, x->gc, BlackPixel(x->dpy, DefaultScreen(x->dpy)));
//		Atom a = XInternAtom(x->dpy, "_NET_WM_WINDOW_TYPE", False);
//		Atom b = XInternAtom(x->dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
//		XChangeProperty (x->dpy, x->win, a, XA_ATOM, 32, PropModeReplace, (unsigned char *) &b, 1);
		XRectangle rect = {0,0,0,0};
		XserverRegion region = XFixesCreateRegion(x->dpy,&rect, 1);
		XFixesSetWindowShapeRegion(x->dpy, x->win, ShapeInput, 0, 0, region);
		XFixesDestroyRegion(x->dpy, region);

//		MWMHints mwmhints;
//	  Atom prop;
//	  memset(&mwmhints, 0, sizeof(mwmhints));
//	  prop = XInternAtom(display, "_MOTIF_WM_HINTS", False);
//	  mwmhints.flags = MWM_HINTS_DECORATIONS;
//	  mwmhints.decorations = 0;
//	  XChangeProperty(x->dpy, x->win, prop, prop, 32, PropModeReplace, (unsigned char *) &mwmhints, PROP_MWM_HINTS_ELEMENTS);
	}

	{ // Initialise the stroke buffer
		TabletStrokes* s = &tablet->strokes;
		s->n = 2048;
		s->buffer = (pt_t*) malloc(sizeof(pt_t) * s->n);
		s->ptr = s->buffer;
	}

	{ // instantiate the engine
		// TODO select from config
		tablet->engine = &engZinnia;
		tablet->engine_ud = engZinnia.Create(&tablet->conf);
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
					XMapWindow(x->dpy, x->win);
					 XFlush(x->dpy);
					TabletStrokes* s = &tablet->strokes;
					if(s->ptr > s->buffer && PT_ISVALID(s->ptr[-1]) && PT_ISVALID(pt)) { //we have at least 2 valid new points
						XDrawLine(x->dpy, x->win, x->gc,
									 s->ptr[-1].x * (float) x->w / (float) d->drv->x_max,
									 s->ptr[-1].y * (float) x->h / (float) d->drv->y_max,
									 pt.x * (float) x->w / (float) d->drv->x_max,
									 pt.y * (float) x->h / (float) d->drv->y_max);
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
