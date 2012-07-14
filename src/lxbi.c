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
#include "driver.h"

// lxbi.c
// This is a simple tablet driver

typedef enum { LXBI_UP, LXBI_DOWN } LxbiState;

void* LxbiCreate() {
	LxbiState* st = fcitx_utils_new(LxbiState);
	*st = LXBI_UP;
	return st;
}

void LxbiDestroy(void* ud) {

}

FcitxTabletDriverEvent LxbiGetEvent(void* ud, const char* buffer, pt_t* pt) {
	LxbiState* st = (LxbiState*) ud;
	static const unsigned short XMASK = 0x281a;
	static const unsigned short YMASK = 0x3010;
	short x = ( (buffer[2]<<8) | (0xff&buffer[1]) ) ^ XMASK;
	short y = ( (buffer[4]<<8) | (0xff&buffer[3]) ) ^ YMASK;
	if(x == 0 && y == 0) {
		if(*st == LXBI_UP) return EV_NONE;
		*st = LXBI_UP;
		return EV_PENUP;
	}
	if(*st == LXBI_UP) {
		*st = LXBI_DOWN;
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
	1437,
	1230,
	LxbiGetEvent,
	LxbiDestroy
};

