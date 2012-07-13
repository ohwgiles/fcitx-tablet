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

#include "driver.h"

// lxbi.c
// This is a simple tablet driver

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
