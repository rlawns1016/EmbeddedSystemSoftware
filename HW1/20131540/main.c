#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>


#define DOWN 1
#define BACK 158
#define PROC 116
#define VOL_UP 115
#define VOL_DOWN 114
#define FND_BASE 12
#define STR_BASE 16
#define DOT_BASE 49
#define NUM_OF_MODE 4
#define BNUM 9

typedef struct
{
	int nowMode;
	struct tm *startTime;
	time_t elapsedTime;
	int strIdx;
	int lval;
	int dotIdx;
	int counter;
	unsigned char fnd[4];
	unsigned char str[33];
}Vals, *ValsPointer;


typedef struct
{
	int r;
	int c;
	int counter;
	int on;
	unsigned char display[10];
	unsigned char checkList[10];
}DVals, *DvalsPointer;

typedef struct
{
	long mtype; // > 0
	int buff[256];
}msg, *msgPointer;


/*********************************************************************
 * 함수명	:  modify
 * param	: addr : 변경할 data의 주소
 * 	           idx : data의 해당 index 번호
 *          	   flg : flag
 * 기능		: flag가 1이면 해당 bit을 1로, 0이면 bit을 0으로 만든다.
 * return	: void
 * ******************************************************************/
void modify(unsigned char *addr, int idx, int flg)
{
	if(flg) (*addr) ^= (1 << idx);
	else (*addr) &= ~(1 << idx);
}


/*********************************************************************
 * 함수명	: ValsInit
 * param	: v : 모드 1,2,3,4에서 사용하는 변수들을 담은 구조체의 포인터
 * 	          mode : 현재 모드
 * 기능		: 현재 모드에 맞춰 v에 담긴 변수들을 초기화한다.
 * return	: void
 * ******************************************************************/
void ValsInit( ValsPointer v, int mode )
{
	time_t t = time(NULL);	

	v->nowMode = mode;
	v->strIdx = -1;
	v->lval = 0;
	v->dotIdx = 0;
	v->counter = 0;
	v->startTime = localtime(&t);
	time(&(v->elapsedTime));
	memset(v->fnd, 0, sizeof(v->fnd));
	memset(v->str, 0, sizeof(v->str));
	memset(v->str, ' ', sizeof(v->str)-1);

	if( mode == 2 ){
		v->lval = 10;
	}
	if( mode == 3 ){
		v->dotIdx = 1;		
	}

}

/*********************************************************************
 * 함수명	: DvalsInit
 * param	: dv : 모드4에서만 사용하는 변수들을 담은 구조체의 포인터
 * 기능		: dv에 담긴 변수들을 초기화한다.
 * return	: void
 * ******************************************************************/
void DValsInit( DvalsPointer dv )
{
	dv->r = 1;
	dv->c = 0;
	dv->on = 1;
	dv->counter = 0;
	memset(dv->display, 0, sizeof(dv->display));
	memset(dv->checkList, 0, sizeof(dv->checkList));
}

/*********************************************************************
 * 함수명	: mode1
 * param	: sw : SW 스위치의 입력 state가 저장된 배열
 * 	           v : 모드 1,2,3,4에서 사용하는 변수들을 담은 구조체의 포인터
 * 기능		: 모드가 1일때 수행되는 process
 * return	: void
 * ******************************************************************/
void mode1(int sw[], ValsPointer v)
{
	static unsigned char hour_inc;
	static unsigned char min_inc;
	unsigned char hour, min;

	time_t t = time(NULL);

	if( sw[0] == DOWN ){
		v->lval = !(v->lval);
	}
	v->startTime = localtime(&t);

	if( v->lval == 1 ){	//change mode
		if( sw[1] == DOWN ){	//reset
			hour_inc = 0;	
			min_inc = 0;
			v->startTime = localtime(&t);
		}
		if( sw[2] == DOWN ){	//hour++
			++hour_inc;
		}
		if( sw[3] == DOWN ){	//min++
			++min_inc;
		}
	}

	hour = v->startTime->tm_hour + hour_inc;
	min = v->startTime->tm_min + min_inc;
	///////////////////////////////////////////
	if( min == 60 ){
		hour++;
		min = 0;
	}
	if( hour == 24 ){
		hour = 0;
	}
	/////////////////////////////////////////////
	v->fnd[0] = hour / 10;	v->fnd[1] = hour % 10;
	v->fnd[2] = min / 10;	v->fnd[3] = min % 10;
}

/*********************************************************************
 * 함수명	: mode2
 * param	: sw : SW 스위치의 입력 state가 저장된 배열
 * 	           v : 모드 1,2,3,4에서 사용하는 변수들을 담은 구조체의 포인터
 * 기능		: 모드가 2일때 수행되는 process
 * return	: void
 * ******************************************************************/
void mode2(int sw[], ValsPointer v)
{
	int t;
	if(v->lval == 0 ){
		printf("divide by zero\n");
		return ;
	}

	if( sw[0] == DOWN ){	//진법 변환
		if( v->lval == 2 )
			v->lval = 10;
		else if( v->lval == 4 )
			v->lval = 2;
		else if( v->lval == 8)
			v->lval = 4;
		else
			v->lval = 8;
	}

	if( sw[3] == DOWN )	v->counter += 1;
	if( sw[2] == DOWN )	v->counter += (v->lval);	
	if( sw[1] == DOWN )	v->counter += (v->lval)*(v->lval);

	t = v->counter;

	v->fnd[3] = t % (v->lval);

	t /= (v->lval);
	v->fnd[2] = t % (v->lval);

	t /= (v->lval);
	v->fnd[1] = t % (v->lval);
}

/*********************************************************************
 * 함수명	: mode3
 * param	: sw : SW 스위치의 입력 state가 저장된 배열
 * 	           v : 모드 1,2,3,4에서 사용하는 변수들을 담은 구조체의 포인터
 * 기능		: 모드가 3일때 수행되는 process
 * return	: void
 * ******************************************************************/
void mode3(int sw[], ValsPointer v)
{	
	int i, n = -1, m = -1, t;
	static int cnt = 0, prev = -1;
	static char c;


	char text[9][3] = 
	{	
		{'.','Q','Z'}, {'A','B','C'}, {'D','E','F'},
		{'G','H','I'}, {'J','K','L'}, {'M','N','O'},
		{'P','R','S'}, {'T','U','V'}, {'W','X','Y'}
	};

	for(i = 0; i < 9; i++){
		if( sw[i] ){
			if( n < 0 )
				n = i;	//first in
			else
				m = i;	//second in
		}
	}

	if( n == 4 && m == 5 ){	//A -> 1, 1 -> A
		v->counter++;
		v->dotIdx = !(v->dotIdx);		
	}
	else if( n == 1 && m == 2 ){	//clear str
		v->counter++;
		v->strIdx = -1;	cnt = 0; prev = -1;
		memset(v->str, ' ', sizeof(unsigned char) * 32);		
	}

	else if( n == 7 && m == 8 ){	//' '
		v->counter++;
		v->strIdx++;			
		c = ' ';
		prev = -1;	cnt = 0;
	}
	else{
		if( n != -1 ){
			v->counter++;
			if( v->dotIdx ){	//알파벳 모드
				if( n != prev ){
					v->strIdx++;
					prev = n;
					cnt = 0;
				}
				else{
					cnt++;
					cnt %= 3;
				}

				c = text[n][cnt];
			}
			else{	//숫자 모드
				v->strIdx++;
				c = (n + 1) + '0';
			}
		}
	}

	t = v->counter % 10000;
	v->fnd[0] = t / 1000;

	t %= 1000;
	v->fnd[1] = t / 100;

	t %= 100;
	v->fnd[2] = t / 10;

	t %= 10;
	v->fnd[3] = t;

	if( v->strIdx > 7 ){
		v->strIdx = 7;
		for( i = 0; i < 7; i++) 	v->str[i] = v->str[i + 1];
	}

	if( v->strIdx != -1 ){
		v->str[ v->strIdx ] = c;
	}
}

/*********************************************************************
 * 함수명	: mode4
 * param	: sw : SW 스위치의 입력 state가 저장된 배열
 * 	           v : 모드 1,2,3,4에서 사용하는 변수들을 담은 구조체의 포인터
 * 		  dv : 모드4에서만 사용하는 변수들을 담은 구조체의 포인터
 * 기능		: 모드가 4일때 수행되는 process
 * return	: void
 * ******************************************************************/
void mode4(int sw[], ValsPointer v, DvalsPointer dv)
{
	static int flg = 0;
	int i,j,t;
	time_t ct;

	time(&ct);

	if( (ct - v->elapsedTime) >= 1 ){	//1초마다 커서 깜박거리게
		flg = !flg;
		v->elapsedTime = ct;		
	}

	if( sw[0] == DOWN ){	//reset
		dv->counter = 0;
		dv->c = 0;
		dv->r = 1;
		memset(dv->checkList, 0, sizeof(dv->checkList));		
	}

	if( sw[2] == DOWN ){	//on off
		dv->counter++;
		dv->on = !(dv->on);		
	}

	if( sw[4] == DOWN ){	//choice
		dv->counter++;
		modify( &(dv->checkList[dv->c]), 7-dv->r, 1 );
	}
	if( sw[6] == DOWN ){	//clear
		dv->counter++;
		memset(dv->checkList, 0, sizeof(dv->checkList));
	}
	if( sw[8] == DOWN ){	//flip
		int i;
		dv->counter++;
		for(i=0; i<10; i++)
		{
			dv->checkList[i] ^= 0xff;
		}		
	}

	if( sw[1] == DOWN ){	//1, 3, 5, 7 -> move cursor
		dv->counter++;
		flg = 1;
		dv->c--;
		if( dv->c < 0 )	dv->c = 0;
		v->elapsedTime = ct;

	}
	if( sw[3] == DOWN ){
		dv->counter++;
		flg = 1;
		dv->r--;
		if( dv->r < 1 )	dv->r = 1;
		v->elapsedTime = ct;
	}memcpy( dv->display, dv->checkList, sizeof(dv->display) );

	if( dv->on == 1 ) modify( &(dv->display[dv->c]), 7-dv->r, flg );
	if( sw[5] == DOWN ){
		dv->counter++;
		flg = 1;
		dv->r++;
		if( dv->r > 7 )	dv->r = 7;
		v->elapsedTime = ct;
	}
	if( sw[7] == DOWN ){
		dv->counter++;
		flg = 1;
		dv->c++;
		if( dv->c > 9 )	dv->c = 9;
		v->elapsedTime = ct;
	}

	t = dv->counter % 10000;
	v->fnd[0] = t / 1000;

	t %= 1000;
	v->fnd[1] = t / 100;

	t %= 100;
	v->fnd[2] = t / 10;

	t %= 10;
	v->fnd[3] = t;


	//data -> output
	memcpy( dv->display, dv->checkList, sizeof(dv->display) );

	if( dv->on == 1 ) modify( &(dv->display[dv->c]), 7-dv->r, flg );

}



int main(){
	int PID_in, PID_out;
	key_t key_in = 1001;
	key_t key_out = 1002;
	int que_id_in, que_id_out;
	msg m_in, m;
	int res;
	int i;

	if( (PID_in = fork()) < 0 ){
		perror("fork error");
		exit(1);
	}

	else if( PID_in == 0 )
	{ 
		char *argv[] = {"./input", NULL};
		execv(argv[0], argv);
		printf("fork input\n");
	}
	else{	//processing with message from input and send message to output
		int child, status;
		int mode = 1;
		int input, sw[9];
		long msec;
		struct timeval start, end;
		Vals v;
		DVals dv;

		que_id_in = msgget(key_in, IPC_CREAT | 0600);
		que_id_out = msgget(key_out, IPC_CREAT | 0600);
		m.mtype = 1002;
		m_in.mtype = 1001;
		gettimeofday(&start, NULL);
		ValsInit( &v, mode );
		DValsInit( &dv );
		//init m
		m.buff[0] = 0;
		for(i=0; i<BNUM; i++){
			m.buff[i + 1] = 0;
		}
		m.buff[10] = v.nowMode;
		m.buff[11] = v.lval;
		for(i=0; i<4; i++){
			m.buff[ FND_BASE + i] = v.fnd[i];
		}
		for(i=0; i<32; i++){
			m.buff[ STR_BASE + i] = v.str[i];
		}
		m.buff[48] = v.dotIdx;
		for(i=0; i<10; i++){
			m.buff[DOT_BASE + i] = dv.display[i];
		}
		if( (PID_out = fork()) < 0 ){
			perror("fork error");
			exit(1);
		}
		else if( PID_out == 0 ){ 
			char *argv[] = {"./output", NULL};
			execv(argv[0], argv);
			printf("fork output\n");
		}

		res = msgsnd(que_id_out, (void*)&m, sizeof(m), IPC_NOWAIT);

		while(1){
			msec = 0;
			input = 0;
			memset(sw, 0, sizeof(sw));
			m.buff[0] = 0;
			for(i=0; i<BNUM; i++){
				m.buff[i + 1] = 0;
			}
			res = msgrcv(que_id_in, &m_in, sizeof(m_in), 1001, IPC_NOWAIT);//NONBLOCK HERE
			if(res > 0){
				memcpy(m.buff, m_in.buff, sizeof(int) * 10);
				//clear m_in
				memset(m_in.buff, 0, sizeof(int) * 10);
			}

			input = m.buff[0];
			for(i=0; i<BNUM; i++){
				sw[i] = m.buff[i + 1];
			}

			gettimeofday(&end, NULL);

			msec = (end.tv_sec-start.tv_sec)*1000+(end.tv_usec-start.tv_usec)/1000;
			if( msec >= 200 ){
				start = end;
			}
			else{
				continue;
			}


			if( input == VOL_UP ){
				mode %= NUM_OF_MODE;
				mode++;	//start by 1
				ValsInit( &v, mode );
				DValsInit( &dv );				
			}
			else if( input == VOL_DOWN ){
				mode--;
				if( mode < 1 )
					mode =  NUM_OF_MODE;
				ValsInit( &v, mode );
				DValsInit( &dv );				
			}

			else if( input == PROC ){
				/* nothing to do */
			}

			else if( input == BACK ){
				break;
			}

			switch(mode){
				case 1:
					mode1(sw, &v);
					break;
				case 2:
					mode2(sw, &v);
					break;
				case 3:
					mode3(sw, &v);
					break;
				case 4:
					mode4( sw, &v, &dv );
					break;
				default :
					break;
			}	

			m.buff[10] = v.nowMode;
			m.buff[11] = v.lval;
			for(i=0; i<4; i++){
				m.buff[ FND_BASE + i] = v.fnd[i];
			}
			for(i=0; i<32; i++){
				m.buff[ STR_BASE + i] = v.str[i];
			}
			m.buff[48] = v.dotIdx;

			for(i=0; i<10; i++){
				m.buff[DOT_BASE + i] = dv.display[i];
			}
			//send msg to output process
			res = msgsnd(que_id_out, (void*)&m, sizeof(m), IPC_NOWAIT);
		}
		kill(PID_in, SIGTERM);
		kill(PID_out, SIGTERM); 
		while( (child = wait(&status)) > 0 );
	}    
	return 0;
}

