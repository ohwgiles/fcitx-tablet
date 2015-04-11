#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits.h>

int main(int argc, char** argv) {
	if(argc != 2) {
		fprintf(stderr, "Usage: %s DEVICE\n", argv[0]);
		return 1;
	}
	char* device = argv[1];
	int fd = open(device, O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Could not open %s for reading\n", device);
		return 2;
	}

	char buffer[8];
	short xmin = SHRT_MAX,ymin=SHRT_MAX,xmax=SHRT_MIN,ymax=SHRT_MIN;
	printf("Calibration: Draw around the edges of the tablet screen\n");

	if(read(fd,buffer,8) < 0) {
		perror("read");
		return 3;
	}

	printf("Detected input. Release pen to end calibration\n");
	while(1) {
		short x = ((buffer[2]<<8)|(0xff&buffer[1]))^0x281a;
		short y = ((buffer[4]<<8)|(0xff&buffer[3]))^0x3010;
		if(x == 0 && y == 0)
			break;
		if(x > xmax) xmax = x;
		if(x < xmin) xmin = x;
		if(y > ymax) ymax = y;
		if(y < ymin) ymin = y;
		read(fd,buffer,8);
	}
	printf("Calibration done. Results: x:(%d,%d) y:(%d,%d)\n", xmin, xmax, ymin, ymax);
	return 0;

}

