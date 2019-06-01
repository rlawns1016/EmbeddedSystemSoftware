#include <linux/input.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>
#include <dirent.h>
#include<sys/msg.h>
#include<string.h>

#define KEY_PRESS 1
#define KEY_RELEASE 0
#define MAX_SIZE_OF_BUFF 64
#define NUM_OF_BUTTON 9

typedef struct
{
	long mtype; // > 0
	int buff[256];
}msg, *msgPointer;


unsigned char q = 0;

void user_signal1(int sig){
	q = 1;
}

int main(int argc, char* argv[]){

	char *filename_ev = "/dev/input/event0";
	char *filename_sw = "/dev/fpga_push_switch";
	int fd_ev, fd_sw, i;
	unsigned char sw[NUM_OF_BUTTON];
	struct input_event evlist[MAX_SIZE_OF_BUFF];
	int flag = 0;
	key_t key = 1001;
	int que_id;
	msg m;
	int res;

	que_id = msgget(key, IPC_CREAT | 0600);
	m.mtype = 1001;

	if( (fd_ev = open(filename_ev, O_RDONLY | O_NONBLOCK)) < 0 ){
		close(fd_ev);
		perror("File Open Error");
		exit(1);
	}

	if( (fd_sw = open(filename_sw, O_RDWR)) < 0 ){
		close(fd_sw);
		perror("File Open Error");
		exit(1);
	}

	(void)signal(SIGINT, user_signal1);

	while(1)
	{
		int r;
		flag = 0;
		memset(sw, 0, sizeof(sw));
		memset(m.buff, 0, sizeof(int) * 10);
		if( (r = read(fd_ev, evlist, sizeof(struct input_event) * MAX_SIZE_OF_BUFF)) >= sizeof(struct input_event) ){
			int value = evlist[0].value;
			if( value == KEY_PRESS ){
				flag = 1;
				m.buff[0] = evlist[0].code;
			}
		}

		read(fd_sw, &sw, sizeof(sw));	
		for( i = 1; i <= NUM_OF_BUTTON; i++){
			m.buff[i] = sw[i - 1];
			if(m.buff[i]){
				flag = 1;
			}
		}

		if(flag){
			res = msgsnd(que_id, (void*)&m, sizeof(m), IPC_NOWAIT);
			//usleep(300000);
		}
	} 


	close(fd_ev);
	close(fd_sw);
	return 0;
}
