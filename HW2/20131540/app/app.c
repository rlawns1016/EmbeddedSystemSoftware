#include<unistd.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<syscall.h>
/************************************************************************************
	My Systemcall Number : 376
	sys_newcall(int term, int cnt, const char __user *option)	
	return : 

	4 byte stream (integer)
                 __________________________________________________________________
                |start loc of fnd | start val of fnd | time term     | # of print  |         
                |_________________|__________________|_______________|_____________|
                4                 3                  2               1             0
                (byte offset)

************************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

////////////////////////////Same with Device Driver///////////////////////////
#define MAJOR_NUM 242
#define GET_DATA 1	//not use in here
#define PUT_DATA 2
#define IOCTL_SET_NUM _IOW(MAJOR_NUM, PUT_DATA, int *)	
//////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]){
	int term, cnt, sys_res, fd, ioctl_res;
	char option[4];
	char * filename = "/dev/dev_driver";


	if(argc != 4) {
		printf("Wrong Input\n");
		return -1;
	}

	term = atoi(argv[1]);
	cnt = atoi(argv[2]);
	strncpy(option, argv[3], 4);
	
	//using system call
	sys_res = syscall(376, term, cnt, option);
	//valid check
	if(sys_res < 0){
		printf("Wrong Input\n");
		return -1;
	}

	//open device file
	fd = open(filename, 0);
	if(fd < 0){
		printf("Device File Open Error\n");
		return -1;
	}

	//using ioctl
	printf("sys_res in app : %d\n", sys_res);

	ioctl_res = ioctl(fd, IOCTL_SET_NUM, &sys_res);
	if(ioctl_res < 0){
		printf("Ioctl failed\n");
		return -1;
	}

	close(fd);
	
	return 0;
}
