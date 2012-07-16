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
#include <sys/stat.h>
#include <fcntl.h>

typedef struct {
	int fd_strokes[2];
	int fd_candidates[2];
	float xsf;
	float ysf;
	char* buffer;
	int bufsize;
} RecogForkData;

void* RecogForkCreate(FcitxTabletConfig* cfg, int x_max, int y_max) {
	RecogForkData* d = fcitx_utils_new(RecogForkData);
	d->xsf = 254.0 / x_max;
	d->ysf = 254.0 / y_max;
	// todo read path to extern app from cfg
	pid_t pid;
	// open pipes
	if(pipe(d->fd_strokes) < 0)
		return FcitxLog(ERROR, "pipe failed"), NULL;
	if(pipe(d->fd_candidates) < 0)
		return FcitxLog(ERROR, "pipe failed"), NULL;
	// fork
	if((pid = fork()) < 0)
		return FcitxLog(ERROR, "fork failed"), NULL;

	if(pid == 0) { // child
		close(d->fd_candidates[0]);
		close(d->fd_strokes[1]);
		FcitxLog(ERROR, "stdout_fileno: %d", STDOUT_FILENO);
		if(d->fd_candidates[1] != STDOUT_FILENO) {
			dup2(d->fd_candidates[1], STDOUT_FILENO);
			close(d->fd_candidates[1]);
		}
		if(d->fd_strokes[0] != STDIN_FILENO) {
			dup2(d->fd_strokes[0], STDIN_FILENO);
			close(d->fd_strokes[0]);
		}
		if(execl("./ccpipe","./ccpipe", NULL) < 0)
			return FcitxLog(ERROR, "execl failed"), NULL;
		//shouldn't get here
		exit(1);
	}

	close(d->fd_candidates[1]);
	close(d->fd_strokes[0]);

	d->buffer = (char*) malloc(1024);
	d->bufsize = 1024;
	return d;
}

void RecogForkDestroy(void* ud) {

}

char* RecogForkProcess(void* ud, pt_t* points, int nPoints) {
	RecogForkData* d = (RecogForkData*) ud;
	short msgsize = nPoints * 2;
	if(d->bufsize < msgsize+2) {
		d->bufsize += 1024;
		d->buffer = (char*) realloc(d->buffer, d->bufsize);
	}
	((short*) d->buffer)[0] = msgsize;

	char* p = &d->buffer[2];
	for(int i=0; i<nPoints; ++i) {
		if(!PT_ISVALID(points[i])) {
			FcitxLog(WARNING, "writing break pt");
			*p++ = 0xff;
			*p++ = 0xff;
		} else {
			*p++ = d->xsf * points[i].x;
			*p++ = d->ysf * points[i].y;
		}
	}
	write(d->fd_strokes[1], d->buffer, msgsize+2);
	int ofd = open("/tmp/strokes.dat", O_CREAT|O_TRUNC|O_WRONLY);
	write(ofd, d->buffer, msgsize+2);
	close(ofd);


//	return 0;

	char result[30];
	short sz = 0;
	if(read(d->fd_candidates[0], &sz, 2) != 2)
		FcitxLog(ERROR, "Couldn't read length");
	FcitxLog(WARNING, "Starting read");
	int n = read(d->fd_candidates[0], result, sz);
	if(n < 0) perror("read candidates");

	FcitxLog(WARNING, "received %d bytes (expecting %d)", n, sz);
	FcitxLog(WARNING, "str: %s", result);
	return 0;
}

FcitxTabletRecogniser recogfork = {
	RecogForkCreate,
	RecogForkProcess,
	RecogForkDestroy
};


