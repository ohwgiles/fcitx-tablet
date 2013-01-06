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
#include <time.h>
#include <sys/types.h>
#include <sys/timerfd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shapeconst.h>
#include <X11/Xcms.h>

#include <fcitx/keys.h>
#include <fcitx/candidate.h>
#include <fcitx/context.h>
#include <fcitx/instance.h>
#include <fcitx-utils/log.h>
#include <fcitx-config/hotkey.h>
#include <fcitx/module.h>
#include <fcitx/module/x11/x11stuff.h>
#include <fcitx/module/x11/fcitx-x11.h>
#include <fcntl.h>

#include "config.h"
#include "driver.h"
#include "point.h"
#include "engine.h"
#include "config.h"

#define FCITX_TABLET_NAME "fcitx-tablet"

// The tablet module consists of two inter-related parts, a fcitx-ime
// and a fcitx-module. The ime is necessary to display candidates and
// forward the chosen candidates to the client application. The event
// module is necessary to select on events from the hardware devices
// and forward such events to the ime for processing. They share one
// data structure, which is created in the event module since it is
// more long-lived than the IME class.

// Drivers, see driver.h
extern FcitxTabletDriver lxbi;

// Engines, see engine.h
extern TabletEngine engFork;
extern TabletEngine engZinnia;

// The Fcitx Tablet module data members
typedef struct {
	// Configuration file
	FcitxTabletConfig config;
	// Members for xlib manipulation
	Display* xDisplay;
	Window xWindow;
	int xWidth,xHeight;
	GC xGC;
	// Members for driver access
	FcitxTabletDriver* driverInstance;
	void* driverData; // the driver's persistent data
	char* driverPacket; // buffer for a packet from the driver
	// Members for stroke data storage
	pt_t* strokesBuffer; // start of buffer
	pt_t* strokesPtr; // moving pointer
	unsigned strokesBufferSize;
	boolean candidatesPending;
	// timeout when inputting a stroke
	int timeoutFd;
	struct itimerspec delay;
	// Recognition engine
	TabletEngine* engineInstance;
	void* engineData;
	// The main fcitx instance
	FcitxInstance* fcitx;
} FcitxTablet;

// Possible actions for the input method, triggered by tablet events
// such as timeouts and penups. See FcitxTabletProcess
typedef enum {
	IME_DISPLAYCANDIDATES,
	IME_COMMIT
} ImeAction;

static boolean CharacterInProgress(FcitxTablet* tablet) {
	// a character drawing is in process if the current position pointer
	// is not at the beginning of the stroke buffer
	return tablet->strokesPtr != tablet->strokesBuffer;
}

// used to cancel a running timer
static struct itimerspec empty_timer = { {0, 0}, {0, 0} };

// Clears the little window and resets the stroke buffer
static void ClearCharacter(FcitxTablet* tablet, boolean cancelPending) {
	tablet->strokesPtr = tablet->strokesBuffer;
	//XClearWindow(tablet->xDisplay, tablet->xWindow);
	tablet->candidatesPending = !cancelPending;
	// clear the timer
	timerfd_settime(tablet->timeoutFd, 0, &empty_timer, NULL);
	// destroy the popup window
	XUnmapWindow(tablet->xDisplay, tablet->xWindow);
	XFlush(tablet->xDisplay);
}

// This function is called from within fcitx. When the user selects a
// candidate, this function copies the actual utf-8 string into the
// output buffer fcitx uses to send to the client
INPUT_RETURN_VALUE FcitxTabletGetCandWord(void* arg, FcitxCandidateWord* candWord) {
	FcitxTablet* tablet = (FcitxTablet*) candWord->owner;
	FcitxInputState *input = FcitxInstanceGetInputState(tablet->fcitx);
	strcpy(FcitxInputStateGetOutputString(input), candWord->strWord);
	return IRV_COMMIT_STRING;
}

// This function is called from within fcitx. It is called when the
// list of input candidates has changed. It extracts the candidates
// from the recognition engine and puts them into the format required
// by fcitx for displaying in its popup window
INPUT_RETURN_VALUE FcitxTabletGetCandWords(void* arg) {
	FcitxTablet* tablet = (FcitxTablet*) arg;
	FcitxInputState *input = FcitxInstanceGetInputState(tablet->fcitx);
	FcitxInstanceCleanInputWindow(tablet->fcitx);
	char* c = tablet->engineInstance->GetCandidates(tablet->engineData);
	int len = strlen(c);
	int i = 0;
	do {
		int n = mblen(&c[i], len);
		if(n <= 0) break;
		FcitxCandidateWord cw;
		cw.callback = FcitxTabletGetCandWord;
		cw.strExtra = NULL;
		cw.priv = NULL;
		cw.owner = tablet;
		cw.wordType = MSG_OTHER;
		// TODO does fcitx free this?
		cw.strWord = (char*) malloc(n+1);
		memcpy(cw.strWord, &c[i], n);
		cw.strWord[n] = 0;
		FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &cw);
		i += n;
	} while(1);
	return IRV_DISPLAY_CANDWORDS;
}

static void CommitFirstCandidate(FcitxTablet* tablet) {
	char s[5]; // five chars should be plenty to hold a utf-8 char
	char* candidates = tablet->engineInstance->GetCandidates(tablet->engineData);
	int l = mblen(candidates, 10);
	memcpy(s, candidates, l);
	s[l] = '\0';
	FcitxInstanceCommitString(tablet->fcitx, FcitxInstanceGetCurrentIC(tablet->fcitx), s);
	timerfd_settime(tablet->timeoutFd, 0, &empty_timer, NULL);
}

// This function is called from within fcitx. It is called when user
// input is detected. This may be a keypress, or it may be triggered by
// a new stroke on the tablet (see FcitxTabletProcess)
INPUT_RETURN_VALUE FcitxTabletDoInput(void* arg, FcitxKeySym sym, unsigned int action) {
	// The event module uses VoidSymbol as a trigger, this means other input methods
	// will ignore it, actual actions will be passed in the action variable
	FcitxTablet* tablet = (FcitxTablet*) arg;
	if(sym == FcitxKey_VoidSymbol) {
		// Depending on the tablet driver, we could add more actions, for
		// example selecting candidates via the tablet
		switch(action) {
		case IME_DISPLAYCANDIDATES:
			tablet->candidatesPending = true;
			return IRV_DISPLAY_CANDWORDS;
			break;
		default:
			FcitxLog(ERROR, "IME asked to perform unknown action");
		}
	}
	// Backspace and spacebar need to be handled specially
	if(FcitxHotkeyIsHotKey(sym, action, FCITX_BACKSPACE)) {
		if(CharacterInProgress(tablet)) {
			// scrap the current character
			ClearCharacter(tablet, true);
			// hide candidate window
			return IRV_CLEAN;
		} else
			return IRV_TO_PROCESS;
	} else if(FcitxHotkeyIsHotKey(sym, action, FCITX_SPACE) || FcitxHotkeyIsHotKey(sym, action, FCITX_ENTER)) {
		if(CharacterInProgress(tablet)) {
			// commit the current character
			CommitFirstCandidate(tablet);
			ClearCharacter(tablet, true);
			return IRV_CLEAN;
		} else if(tablet->candidatesPending)
			return IRV_CLEAN;
		else
			return IRV_TO_PROCESS;
	}

	if(FcitxHotkeyIsHotKeyDigit(sym, action) && tablet->candidatesPending) {
		int k = FcitxHotkeyCheckChooseKey(sym,action,DIGIT_STR_CHOOSE);
		if(k >= 0) {
			FcitxInputState* state = FcitxInstanceGetInputState(tablet->fcitx);
			FcitxCandidateWordList* candidates = FcitxInputStateGetCandidateList(state);
			if(!CharacterInProgress(tablet))
				FcitxInstanceForwardKey(tablet->fcitx, FcitxInstanceGetCurrentIC(tablet->fcitx), FCITX_PRESS_KEY, FcitxKey_BackSpace, 0);
			ClearCharacter(tablet, true);
			return FcitxCandidateWordChooseByIndex(candidates, k);
		}
	}
	return IRV_TO_PROCESS;
}

// This function is called whenever the client input window is changed,
// as well as on startup. Clean the current character and hide the drawing window
void FcitxTabletReset(void* arg) {
	FcitxTablet* tablet = (FcitxTablet*) arg;
	// reset stroke buffer
	ClearCharacter(tablet, true);
}

void FcitxTabletImeDestroy(void* arg) {
	// don't need to destroy anything since the event module manages it
}

// Creates the IME component
void* FcitxTabletImeCreate(FcitxInstance* instance) {
	// Steal the user data from the event addon, since it's more long lived
	FcitxTablet* ud = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance), FCITX_TABLET_NAME)->addonInstance;
	FcitxInstanceRegisterIM(
				instance,
				ud, //userdata
				"tablet", //uniqueName
				"Tablet", //name
				"tablet", //iconName
				NULL,     //Init
				FcitxTabletReset, //Reset
				FcitxTabletDoInput, //DoInput
				FcitxTabletGetCandWords, //GetCandWords
				NULL, //PhraseTips
				NULL, //Save
				NULL, //ReloadConfig
				NULL, //KeyBlocker
				1, //Priority
				"zh_CN" //langCode
				);
	return ud;
}

// Creates the event addon
void* FcitxTabletCreate(FcitxInstance* instance) {
	FcitxTablet* tablet = fcitx_utils_new(FcitxTablet);
	FcitxTabletLoadConfig(&tablet->config);
	// TODO select driver from config, currently using lxbi

	{ // Initialise the driver
		switch(tablet->config.Driver) {
		case DRIVER_LXBI:
			tablet->driverInstance = &lxbi;
		break;
		// add other drivers here
		}
		tablet->driverData = tablet->driverInstance->Create();
		tablet->driverPacket = (char*) malloc(tablet->driverInstance->packet_size);
	}

	{ // Initialise the X display
		if(NULL == (tablet->xDisplay = FcitxX11GetDisplay(instance)))  {
			FcitxLog(ERROR, "Unable to open X display");
			return NULL;
		}
		// get dimensions
		int screen = DefaultScreen(tablet->xDisplay);
		tablet->xWidth = tablet->config.Width;
		tablet->xHeight = tablet->config.Height;
		int x = tablet->config.XPos > 0 ? tablet->config.XPos : XDisplayWidth(tablet->xDisplay, screen) - tablet->xWidth + tablet->config.XPos;
		int y = tablet->config.YPos > 0 ? tablet->config.YPos : XDisplayHeight(tablet->xDisplay, screen) - tablet->xHeight + tablet->config.YPos;
		// create colours
		XColor back;
		XColor fore;
		{
			char colourString[32];
			{
				int r = (255*tablet->config.BackgroundColour.r);
				int g = (255*tablet->config.BackgroundColour.g);
				int b = (255*tablet->config.BackgroundColour.b);
				sprintf(colourString,"rgb:%x/%x/%x",r,g,b);
			}
			Colormap defaultCMap = DefaultColormap(tablet->xDisplay, screen);
			XParseColor(tablet->xDisplay, defaultCMap, colourString, &back);
			XAllocColor(tablet->xDisplay, defaultCMap, &back);
			{
				int r = (255*tablet->config.StrokeColour.r);
				int g = (255*tablet->config.StrokeColour.g);
				int b = (255*tablet->config.StrokeColour.b);
				sprintf(colourString,"rgb:%x/%x/%x",r,g,b);
			}
			XParseColor(tablet->xDisplay, defaultCMap, colourString, &fore);
			XAllocColor(tablet->xDisplay, defaultCMap, &fore);
		}
		// set window attributes and create window
		XSetWindowAttributes attrs;
		attrs.override_redirect = True;
		attrs.background_pixel = back.pixel;
		tablet->xWindow = XCreateWindow(tablet->xDisplay, DefaultRootWindow(tablet->xDisplay),
			 x, y, tablet->xWidth, tablet->xHeight, tablet->config.BorderWidth, CopyFromParent,
			 InputOutput, CopyFromParent, CWBackPixel | CWOverrideRedirect, &attrs);
		// set up the foreground line (stroke) style
		XGCValues gcv;
		gcv.function = GXcopy;
		gcv.subwindow_mode = IncludeInferiors;
		gcv.line_width = 3;
		gcv.cap_style = CapRound;
		gcv.join_style = JoinRound;
		tablet->xGC = XCreateGC(tablet->xDisplay, tablet->xWindow, GCFunction | GCSubwindowMode | GCLineWidth | GCCapStyle | GCJoinStyle, &gcv);
		XSetForeground(tablet->xDisplay, tablet->xGC, fore.pixel);
		// prevent the window from getting focus or input
		XRectangle rect = {0,0,0,0};
		XserverRegion region = XFixesCreateRegion(tablet->xDisplay,&rect, 1);
		XFixesSetWindowShapeRegion(tablet->xDisplay, tablet->xWindow, ShapeInput, 0, 0, region);
		XFixesDestroyRegion(tablet->xDisplay, region);
	}

	{ // Initialise the stroke buffer
		tablet->strokesBufferSize = 2048; // should be heaps. Will get automatically enlarged if required
		tablet->strokesBuffer = (pt_t*) malloc(sizeof(pt_t) * tablet->strokesBufferSize);
		tablet->strokesPtr = tablet->strokesBuffer;
	}

	{ // instantiate the engine
		switch(tablet->config.Engine) {
		case ENGINE_ZINNIA:
			tablet->engineInstance = &engZinnia;
			break;
		case ENGINE_FORK:
			tablet->engineInstance = &engFork;
			break;
		// add other engines here
		}
		tablet->engineData = tablet->engineInstance->Create(&tablet->config);
	}
	
	tablet->candidatesPending = false;

	{ // set up the timerfd
		tablet->timeoutFd = timerfd_create(CLOCK_MONOTONIC, 0);
		tablet->delay.it_interval.tv_sec = 0;
		tablet->delay.it_interval.tv_nsec = 0;
		tablet->delay.it_value.tv_sec = tablet->config.CommitCharMs / 1000;
		tablet->delay.it_value.tv_nsec = (tablet->config.CommitCharMs % 1000) * 1000000;
	}

	tablet->fcitx = instance;
	return tablet;
}

// Required by fcitx. Adds our file descriptor (for the tablet) to its
// fd_set, so we can get notified about new tablet input
void FcitxTabletSetFd(void* arg) {
	FcitxTablet* tablet = (FcitxTablet*) arg;
	int fd = tablet->driverInstance->GetDescriptor(tablet->driverData);
	if(fd > 0) {
		FD_SET(fd, FcitxInstanceGetReadFDSet(tablet->fcitx));
		if(FcitxInstanceGetMaxFD(tablet->fcitx) < fd)
			FcitxInstanceSetMaxFD(tablet->fcitx, fd);
	}
	// also add the timeout FD
	FD_SET(tablet->timeoutFd, FcitxInstanceGetReadFDSet(tablet->fcitx));
	if(FcitxInstanceGetMaxFD(tablet->fcitx) < tablet->timeoutFd)
		FcitxInstanceSetMaxFD(tablet->fcitx, tablet->timeoutFd);
}

// Adds a new coordinate to the stroke buffer, enlarging if necessary
static void PushCoordinate(FcitxTablet* tablet, pt_t newpt) {
	*tablet->strokesPtr++ = newpt;
	if(tablet->strokesPtr == &tablet->strokesBuffer[tablet->strokesBufferSize]) { // if we overflow the buffer, increase it
		FcitxLog(WARNING, "Resizing stroke buffer");
		int newsize = tablet->strokesBufferSize + 1024;
		pt_t* newbuf = (pt_t*) realloc(tablet->strokesBuffer, sizeof(pt_t)*newsize);
		if(newbuf == NULL)
			FcitxLog(ERROR, "Failed to allocate more stroke memory");
		tablet->strokesBuffer = newbuf;
		tablet->strokesPtr = &tablet->strokesBuffer[tablet->strokesBufferSize];
		tablet->strokesBufferSize = newsize;
	}
}

// Called when we wake up from select, i.e. the tablet has data to read
void FcitxTabletProcess(void* arg) {
	FcitxTablet* tablet = (FcitxTablet*) arg;
	int fd = tablet->driverInstance->GetDescriptor(tablet->driverData);
	// Check we woke up for the right reason
	if(fd > 0 && FD_ISSET(fd, FcitxInstanceGetReadFDSet(tablet->fcitx))) {
		{ // first read a packet from the raw device
			int n = 0;
			const int pktsize = tablet->driverInstance->packet_size;
			do {
				n += read(fd, &tablet->driverPacket[n], pktsize - n);
			} while(n < pktsize);
		}
		boolean redraw = false;
		{ // then send it to the driver to convert into events
			FcitxTabletDriverEvent e;
			pt_t pt;
			while((e = tablet->driverInstance->GetEvent(tablet->driverData, tablet->driverPacket, &pt)) != EV_NONE) {
				// if the tablet did anything, cancel the timer
				timerfd_settime(tablet->timeoutFd, 0, &empty_timer, NULL);
				switch(e) {
				case EV_PENDOWN:
					// do nothing
					break;
				case EV_PENUP: {
					// push an invalid (end-of-stroke) point
					pt_t p = PT_INVALID;
					PushCoordinate(tablet, p);
					// run the recognition engine
					tablet->engineInstance->Process(tablet->engineData, tablet->strokesBuffer, (tablet->strokesPtr - tablet->strokesBuffer));
					// If we're using the IME, show the candidate window
					FcitxInstanceProcessKey(tablet->fcitx, FCITX_PRESS_KEY, 0, FcitxKey_VoidSymbol, IME_DISPLAYCANDIDATES);
					// start the stroke commit timer
					timerfd_settime(tablet->timeoutFd, 0, &tablet->delay, NULL);
				} break;
				case EV_POINT: {
					// If it's not shown already, show the character drawing window
					if(tablet->strokesPtr > tablet->strokesBuffer && PT_ISVALID(tablet->strokesPtr[-1]) && PT_ISVALID(pt)) { //we have at least 2 valid new points
						// draw the line, scaling for the size of the window
						XDrawLine(tablet->xDisplay, tablet->xWindow, tablet->xGC,
									 tablet->strokesPtr[-1].x * (float) tablet->xWidth / (float) tablet->driverInstance->x_max,
									 tablet->strokesPtr[-1].y * (float) tablet->xHeight / (float) tablet->driverInstance->y_max,
									 pt.x * (float) tablet->xWidth / (float) tablet->driverInstance->x_max,
									 pt.y * (float) tablet->xHeight / (float) tablet->driverInstance->y_max);
						redraw = true;
					}
					XWarpPointer(tablet->xDisplay, None, XRootWindow(tablet->xDisplay,XDefaultScreen(tablet->xDisplay)), 0, 0, 0, 0, pt.x, pt.y);
					PushCoordinate(tablet, pt);
				} break;
				default:
					FcitxLog(ERROR, "Driver returned unknown event: %d", e);
					break;
				}
			}
		}

		if(redraw) { // draw the stroke on the screen
			// If it's not shown already, map the win
			XMapWindow(tablet->xDisplay, tablet->xWindow);
			XFlush(tablet->xDisplay);
		}

		FD_CLR(fd, FcitxInstanceGetReadFDSet(tablet->fcitx));
	}

	if(FD_ISSET(tablet->timeoutFd, FcitxInstanceGetReadFDSet(tablet->fcitx))) {
		// the timer expired. Set the flag so that the next pendown will commit the most likely character
		timerfd_settime(tablet->timeoutFd, 0, &empty_timer, NULL);
		FD_CLR(tablet->timeoutFd, FcitxInstanceGetReadFDSet(tablet->fcitx));
		CommitFirstCandidate(tablet);
		ClearCharacter(tablet, false);
	}
}

// Called when the module is closed (fcitx closes)
void FcitxTabletDestroy(void* arg) {
	FcitxTablet* tablet = (FcitxTablet*) arg;
	XFreeGC(tablet->xDisplay, tablet->xGC);
	XCloseDisplay(tablet->xDisplay);
	tablet->driverInstance->Destroy(tablet->driverData);
	free(tablet->driverPacket);
	free(tablet->strokesBuffer);
}

// Instantiate the event module
FCITX_EXPORT_API FcitxModule module = {
	FcitxTabletCreate,
	FcitxTabletSetFd,
	FcitxTabletProcess,
	FcitxTabletDestroy,
	NULL
};

// Instantiate the IME module
FCITX_EXPORT_API FcitxIMClass ime = {
	FcitxTabletImeCreate,
	FcitxTabletImeDestroy
};
