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

#include "recog.h"
#include <fcitx-utils/log.h>
// fork.c
// A recogniser that forks an external program

// The protocol is expected to be as follows. The recogniser will read
// from stdin. fcitx-tablet will send binary data: the first 16 bits is the
// number of bytes in the message, the following message contains (x,y) pairs
// of coordinate points. x and y are 8 bits each. After processing the points
// the recogniser will output a 16 bit integer describing the length of its
// reply, following which it will output its results in UTF-8 in order of
// most likely to least likely. It will then block on stdin waiting for the
// next message.

typedef struct {
	int fd_strokes[2];
	int fd_candidates[2];
	float xsf;
	float ysf;
} RecogForkData;

void* RecogForkCreate(FcitxTabletConfig* cfg, int x_max, int y_max) {
	RecogForkData* d = fcitx_utils_new(RecogForkData);
	d->xsf = 254.0 / x_max;
	d->ysf = 254.0 / y_max;
	// todo read path to extern app from cfg
	pid_t pid;
	// open pipes
	if(pipe(d->fd_strokes) < 0 || pipe(d->fd_candidates) < 0)
		return FcitxLog(ERROR, "pipe failed"), NULL;
	// fork
	if((pid = fork()) < 0)
		return FcitxLog(ERROR, "fork failed"), NULL;

	if(pid == 0) { // child
		if(d->fd_strokes[0] != STDIN_FILENO) {
			dup2(d->fd_strokes[0], STDIN_FILENO);
			close(d->fd_strokes[0]);
		}
		if(d->fd_candidates[1] != STDOUT_FILENO) {
			dup2(d->fd_candidates[1], STDOUT_FILENO);
			close(d->fd_candidates[1]);
		}
		if(execl("./ccpipe","./ccpipe", NULL) < 0)
			return FcitxLog(ERROR, "execl failed"), NULL;
	}

	return 0;
}

void RecogForkDestroy(void* ud) {

}

char* RecogForkProcess(void* ud, pt_t* points, int nPoints) {
	RecogForkData* d = (RecogForkData*) ud;
	write(d->fd_strokes[1], &nPoints, 2);
	for(int i=0; i<nPoints; ++i) {
		char x = d->xsf * points[i].x;
		char y = d->ysf * points[i].y;
		write(d->fd_strokes[1], &x, 1);
		write(d->fd_strokes[1], &y, 1);
	}
	return 0;
}

FcitxTabletRecogniser recogfork = {
	RecogForkCreate,
	RecogForkProcess,
	RecogForkDestroy
};


