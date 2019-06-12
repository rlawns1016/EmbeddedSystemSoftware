#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int main(void){
	int fd;
	int retn;
	char buf[4] = { 0, 0, 0, 0 };		//4byte
	
	fd = open("/dev/bus/usb/002/", O_RDWR);
	if(fd < 0) {
		perror("/dev/bus/usb/002/ error");
		exit(-1);
	}
    else { printf("< inter Device has been detected > \n"); }
	

	retn = write(fd, buf, 2);
	close(fd);

	return 0;
}
