#ifndef FCITX_TABLET_RECOG_H
#define FCITX_TABLET_RECOG_H
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

#include "point.h"

// A recogniser takes a buffer of strokes and converts it into a
// sorted list of UTF-8 candidates

typedef struct {
	void* (*Create)();
	char* (*Process)(void*, pt_t*, int);
	void (*Destroy)(void*);
} FcitxTabletRecogniser;

#endif