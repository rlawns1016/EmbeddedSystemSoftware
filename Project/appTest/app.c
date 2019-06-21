#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#define READ 1
#define WRITE 2
#define EXIT 0

int main(void){
	int fd;
	int retn;
	int input;
	int val;
	unsigned char buf[4] = { 0, 0, 0, 0 };		//4byte
	
	fd = open("/dev/usb/usb2-1.2", O_RDWR);
	if(fd < 0) {
		perror("/dev/usb/usb2-1.2 error");
		exit(-1);
	}
    else { printf("[APP] < inter Device has been detected > \n"); }
	
	printf("\n*********Simple Test*********\n");
	printf("*\t1. READ\n");
	printf("*\t2. WRITE\n");
	printf("*\t0. EXIT\n");
	printf("\n*****************************\n");
	while(1){
		buf[0] = 0;	//clear buff
		printf("Input : ");
		scanf("%d", &input);
		if(input == EXIT)	break;
		else if(input == READ){
			retn = read(fd, buf, 1);
			val = buf[0];
			printf("Read Value (1Byte) : %d\n", val);
		}
		else if(input == WRITE){
			printf("Write Value (1 Byte): ");
			scanf("%d", &val);
			buf[0] = (unsigned char)val;
			retn = write(fd, buf, 1);
			if(retn < 0){
				printf("Write Error\n");
				break;
			}
		}
		else{
			continue;
		}
	}

	//retn = write(fd, buf, 1);

	close(fd);
	
	return 0;
}
