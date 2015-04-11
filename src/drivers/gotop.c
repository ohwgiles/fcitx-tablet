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
#include <sys/stat.h>
#include <fcntl.h>
#include <fcitx-utils/utils.h>
#include <fcitx-utils/log.h>
#include "driver.h"

// gotop.c
// Basic reverse engineered driver
// for tablets with 08f2:6370 Gotop Information Inc
// Tested with
// - LXBI 智能笔 手写识别系统
// - UnisCom CR-360
// See rules/70-gotop.rules

typedef enum { GOTOP_UP, GOTOP_DOWN } GotopState;
typedef struct {
	const char* dev;
	int fd;
	short last_x;
	short last_y;
	GotopState st;
} Gotop;

int GotopGetFD(void* ud) {
	Gotop* gt = (Gotop*) ud;
    if(gt->fd < 0) { // try again to open FD
		gt->fd = open(gt->dev, O_RDONLY);
	}
	return gt->fd;
}

void* GotopCreate(const char* dev) {
	Gotop* gt = fcitx_utils_new(Gotop);
	gt->dev = dev;
    gt->fd = -1;
	gt->st = GOTOP_UP;
    GotopGetFD(gt);
	return gt;
}

void GotopDestroy(void* ud) {
	Gotop* gt = (Gotop*) ud;
	free(gt);
}

FcitxTabletDriverEvent GotopGetEvent(void* ud, const char* buffer, pt_t* pt) {
	Gotop* gt = (Gotop*) ud;
	static const unsigned short XMASK = 0x281a;
	static const unsigned short YMASK = 0x3010;
	short x = ( (buffer[2]<<8) | (0xff&buffer[1]) ) ^ XMASK;
	short y = ( (buffer[4]<<8) | (0xff&buffer[3]) ) ^ YMASK;
	//collapse repeated events
	if(x == gt->last_x && y == gt->last_y) return EV_NONE;
	gt->last_x = x;
	gt->last_y = y;
	if(x == 0 && y == 0) {
		if(gt->st == GOTOP_UP) return EV_NONE;
		gt->st = GOTOP_UP;
		return EV_PENUP;
	}
	if(gt->st == GOTOP_UP) {
		gt->st = GOTOP_DOWN;
		return EV_PENDOWN;
	}
	pt->x = x;
	pt->y = y;
	return EV_POINT;
}

FcitxTabletDriver gotop = {
	GotopCreate,
	GotopGetFD,
	8,
	GotopGetEvent,
	GotopDestroy
};
