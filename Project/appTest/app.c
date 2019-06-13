#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int main(void){
	int fd;
	int retn;
	char buf[4] = { 0, 0, 0, 0 };		//4byte
	
	fd = open("/dev/usb/usb2-1.2", O_RDWR);
	if(fd < 0) {
		perror("/dev/usb/usb2-1.2 error");
		exit(-1);
	}
    else { printf("[APP] < inter Device has been detected > \n"); }
	

	retn = write(fd, buf, 2);

	close(fd);
	
	return 0;
}
