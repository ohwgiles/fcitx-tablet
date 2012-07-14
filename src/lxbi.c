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

#include <fcitx-utils/utils.h>
#include <fcitx-utils/log.h>
#include "driver.h"

// lxbi.c
// This is a simple tablet driver

typedef enum { LXBI_UP, LXBI_DOWN } LxbiState;
typedef struct {
	short last_x;
	short last_y;
	LxbiState st;
} Lxbi;

void* LxbiCreate() {
	Lxbi* lx = fcitx_utils_new(Lxbi);
	lx->st = LXBI_UP;
	FcitxLog(WARNING, "LxbiCreate");
	return lx;
}

void LxbiDestroy(void* ud) {
	Lxbi* lx = (Lxbi*) ud;
	free(lx);
}

FcitxTabletDriverEvent LxbiGetEvent(void* ud, const char* buffer, pt_t* pt) {
	Lxbi* lx = (Lxbi*) ud;
	static const unsigned short XMASK = 0x281a;
	static const unsigned short YMASK = 0x3010;
	short x = ( (buffer[2]<<8) | (0xff&buffer[1]) ) ^ XMASK;
	short y = ( (buffer[4]<<8) | (0xff&buffer[3]) ) ^ YMASK;
	//collapse repeated events
	if(x == lx->last_x && y == lx->last_y) return EV_NONE;
	lx->last_x = x;
	lx->last_y = y;
	//FcitxLog(WARNING, "x: %d, y: %d", x, y);
	//return EV_NONE;
	if(x == 0 && y == 0) {
		if(lx->st == LXBI_UP) return EV_NONE;
		lx->st = LXBI_UP;
		return EV_PENUP;
	}
	if(lx->st == LXBI_UP) {
		lx->st = LXBI_DOWN;
		return EV_PENDOWN;
	}
	// x E (130,1437), y E (394,1230)
	pt->x = x - 130;
	pt->y = y - 394;
	return EV_POINT;
}


FcitxTabletDriver lxbi = {
	LxbiCreate,
	8,
	1307,
	836,
	LxbiGetEvent,
	LxbiDestroy
};

