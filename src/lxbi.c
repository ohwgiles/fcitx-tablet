#include "lxbi.h"


void* LxbiCreate() {
	return 0;
}

void LxbiDestroy(void* ud) {

}

int LxbiPacketSize(void* ud) {
	return 8;
}

FcitxTabletDriverEvent LxbiGetEvent(void* ud, const char* buffer, pt_t* pt) {
	pt->x = 30;
	pt->y = 120;
	return EV_POINT;
}


FcitxTabletDriver lxbi = {
	LxbiCreate,
	LxbiPacketSize,
	LxbiGetEvent,
	LxbiDestroy
};
