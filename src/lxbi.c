#include "lxbi.h"


void* LxbiCreate() {
	return 0;
}

void LxbiDestroy(void* ud) {

}

FcitxTabletDriverEvent LxbiGetEvent(void* ud, const char* buffer, pt_t* pt) {
	static int i = 0;
	i++;
	if(i == 8) { i = 0; return EV_NONE; }
	pt->x = buffer[2*i];
	pt->y = buffer[2*i+1];
	return EV_POINT;
}


FcitxTabletDriver lxbi = {
	LxbiCreate,
	8,
	1234,
	1234,
	LxbiGetEvent,
	LxbiDestroy
};
