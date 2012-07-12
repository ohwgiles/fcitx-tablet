#ifndef FCITX_TABLET_RECOG_H
#define FCITX_TABLET_RECOG_H

#include "point.h"

typedef struct {
	void* (*Create)();
	char* (*Process)(void*, pt_t*);
	void (*Destroy)(void*);
} FcitxTabletRecogniser;

#endif
