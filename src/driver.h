#ifndef FCITX_TABLET_DRIVER_H
#define FCITX_TABLET_DRIVER_H

#include "point.h"

typedef enum {
	EV_NONE,
	EV_PENUP,
	EV_PENDOWN,
	EV_POINT
} FcitxTabletDriverEvent;

typedef struct _FcitxTabletDriver {
	void* (*Create)();
	unsigned packet_size;
	unsigned x_max;
	unsigned y_max;
	FcitxTabletDriverEvent (*GetEvent)(void*, const char*, pt_t*);
	void (*Destroy)(void*);
} FcitxTabletDriver;


#endif
