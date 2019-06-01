#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/msg.h>

#define LED_BASE_ADDR 0x08000000
#define LED_ADDR 0x16

unsigned char fnd[4];		//fnd
unsigned char str[32];		//text lcd
unsigned char display[10];	//dot matrix(mode 4)
unsigned char dotInfo[3][10] = 
{
	{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // '1'
	{0x1c,0x36,0x63,0x63,0x63,0x7f,0x7f,0x63,0x63,0x63}, // 'A'
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 } // blank 
};	// dot matrix(mode1(blanck), mode2(blanck), mode 3('1' or 'A'))

typedef struct
{
  long mtype; // > 0
  int buff[256];
}msg, *msgPointer;



/*********************************************************************
 * 함수명	: msgtoVals
 * param	: m : main으로부터 받은 메세지
 * 		  mode : output process에서의 모드
 * 	          lval : output process에서의 led value
 * 		  dotMat : output process에서의 DotInfo의 Index
 * 기능		: main process으로부터 받은 메시지로 output process에서의 변수들을 셋팅한다.
 * return	: void
 * ******************************************************************/
void msgToVals(msg m, int *mode, int *lval, int *dotIdx){
	int i;
	*mode = m.buff[10];
	*lval = m.buff[11];
	for(i = 0; i < 4; i++){
		fnd[i] = m.buff[i + 12];
	}
	for(i = 0; i < 32; i++){
		str[i] = m.buff[i + 16];
	}
	*dotIdx = m.buff[48];
	
	for(i=0; i<10; i++){
		display[i] = m.buff[i + 49];
	}
	
}


/*********************************************************************
 * 함수명	: writeFnd
 * param	: None
 * 기능		: fnd에 저장된 값들을 fnd의 device file에 쓴다.	(USE DEVICE DRIVER)
 * return	: void
 * ******************************************************************/
void writeFnd()
{
	char *filename_fnd = "/dev/fpga_fnd";
	int fd = open(filename_fnd, O_RDWR);
	ssize_t res;
	
	if( fd < 0 ){
		perror("File Open Error");
		exit(1);
	}

	res = write(fd, &fnd, 4);
	if( res < 0 ){
		perror("File Write Error'");
		exit(1);
	}

	close(fd);
}

/*********************************************************************
 * 함수명	: writeLcd
 * param	: None
 * 기능		: str에 저장된 값들을 lcd의 device file에 쓴다. (USE DEVICE DRIVER)
 * return	: void
 * ******************************************************************/
void writeLcd()

{

           char *filename_lcd = "/dev/fpga_text_lcd";

           //LCD Device File name

           int fd = open(filename_lcd, O_WRONLY);

           //open device file

           ssize_t res;

           //store a result of write function

           if( fd < 0 ){

                     perror("File Open Error");

                     exit(1);

           }

           //handling open error

 

           res = write(fd, str, 32);

           //write 32 characters in str to Lcd Device File

           //str -> global variable in output.c

 

           if( res < 0 ){

                     perror("File Write Error'");

                     exit(1);

           }

           //handling write error

           close(fd);

           //close device file

}
/*********************************************************************
 * 함수명	: writeLed
 * param	: n : 입력된 Led State
 * 기능		: 입력된 Led State의 값을 Led Device Memory에 쓴다. (USE MMAP)
 * return	: void
 * ******************************************************************/
void writeLed(int n)

{

           char *filename_mem = "/dev/mem";

           //Memory device file

           int fd;

           unsigned long *base_addr = 0;

           //logical address which map to physical starting address of Led Device memory

           unsigned char *target_addr = 0;

           //logical address which map to physical target address of Led Device memory

           //want to write n in target address

 

           fd = open(filename_mem, O_RDWR | O_SYNC);

           //open memory device file

           if( fd < 0 ){

                     perror("File Open Error");

                     exit(1);

           }

           //handling open error

           if(  n > 255 || n < 0 ){

                     printf("Long Led Data");

                     close(fd);

                     return ;

           }

           //verify input data

 

           base_addr = (unsigned long*)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, LED_BASE_ADDR);

           //mapping logical address to physical address

 

           if( base_addr == MAP_FAILED ){

                     printf("MMAP Error");

                     close(fd);

                     return ;

           }

           //handling mmap error

 

          

           target_addr = (unsigned char*)((void*)base_addr + LED_ADDR);

           //calculate target address

           *target_addr = n;

           //write n in target address

           munmap(base_addr, 4096); ////////////////////////////////////!!!

           //release memory mapping

           close(fd);

           //close memory device file

}  




/*********************************************************************
 * 함수명	: writeDot
 * param	: idx : 입력된 DotIdx	(mode 1, 2, 3에서 사용)
 * 기능		: 입력된 DotIdx 에 따른 dotInfo data를 Dot Matrix device file에 쓴다 (mode 1, 2, 3) (USE DEVICE DRIVER)
 *                display의 data들을 Dot Matrix device file에 쓴다 (mode 4) (USE DEVICE DRIVER)
 * return	: void
 * ******************************************************************/
void writeDot(int idx, int mode){
	char *filename_dot = "/dev/fpga_dot";
	int fd = open(filename_dot, O_WRONLY);
	ssize_t res;

	if( fd < 0 ){
		perror("File Open Error");
		exit(1);
	}
	if(mode == 1 || mode == 2 || mode == 3)	{
		res = write(fd, dotInfo[idx], sizeof(dotInfo[idx]));
	}

	else{	//mode 4
		res = write(fd, display, sizeof(display));
	}

	if( res < 0 ){
		perror("File Write Error'");
		exit(1);
	}

	close(fd);
}

int main(int argc, char* argv[]){
  	key_t key = 1002;
	int que_id;
	msg m;
  	int mode = 1, lval = 0, dotIdx = 2;
	int res;	
	time_t start, cur;
	
	que_id = msgget(key, IPC_CREAT | 0600);
	time(&start);
	time(&cur);
	
	//get msg
	m.mtype = 1002;
	res = msgrcv(que_id, &m, sizeof(m), 1002, 0);

	if(res > 0)	msgToVals(m, &mode, &lval, &dotIdx);
	
	
	writeFnd();
	writeDot(2, 1);	//blanck
	writeLed(128);	//1번 on
	writeLcd();

	while(1){
		//get msg
	
		res = msgrcv(que_id, &m, sizeof(m), 1002, 0);//block if not recieve from main
		if(res > 0)	msgToVals(m, &mode, &lval, &dotIdx);
		/*
		printf("MODE : %d\n", mode);
		printf("LEDval : %d, FND : %d %d %d %d, ", lval, fnd[0], fnd[1], fnd[2], fnd[3]);	 
		printf("str : %s, ", str);
		
		printf("dotIdx : %d, \n", dotIdx);
		
		*/
		if( mode == 1 ){
			time(&cur);
			writeFnd();
			writeLcd();
			writeDot(2, 1);
			if( cur - start >= 1 ){
				start = cur;
				if( lval == 1 ){	//깜빡임
					static int flag = 0;
					if(flag)	writeLed(32);
					else 	writeLed(16);
					flag = !flag;
				}
				else{
					writeLed(128);
				}
			}
		}
		else if( mode == 2 ){
			time(&cur);
			writeFnd();
			writeLcd();
			writeDot(2, 1);			
			if( cur - start >= 1 ){
				start = cur;
				if( lval == 10 )
					writeLed(64);
				else if( lval == 8 )
					writeLed(32);
				else if( lval == 4 )
					writeLed(16);
				else
					writeLed(128);
			}
		}
		else if( mode == 3 ){
			time(&cur);
			writeFnd();
			writeLcd();
			if( cur - start >= 1 ){
				start = cur;
				writeDot(dotIdx, 3);
			}
			writeLed(0);
		}
		else {	//mode 4
			writeFnd();
			writeLcd();
			writeDot(-1, 4);
			writeLed(0);
		}
	}
	return 0;
}
