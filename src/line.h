#ifndef LINE_H
#define LINE_H
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

// SimplifyLine uses the Douglas-Peucker algorithm to remove extraneous
// points from a user-drawn stroke. It reduces the complexity of the curves
// thus making it easier for recognition engines.
// Pass the address of a pointer into the output parameter, the function
// will allocate memory for it. It is the responsibility of the client
// to free the memory. The return value of the function is the number
// of elements in the allocated output array, or -1 for error.
int SimplifyLine(pt_t* input, int nInput, pt_t** output, int epsilon);

#endif

