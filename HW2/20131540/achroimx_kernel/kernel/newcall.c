#include <linux/kernel.h>
#include <asm/uaccess.h>

/////////////////////////////HW2////////////////////////////



/**************************************************************************************
	  
* Name		: sys_newcall
* params	: term   : time term 
		  cnt    : # of print
		  option : start option
* return	: 4 byte stream
		 __________________________________________________________________
		|start loc of fnd | start val of fnd | time term     | # of print  |         
		|_________________|__________________|_______________|_____________|
                4                 3                  2               1 		   0
		(byte offset)
***************************************************************************************/

asmlinkage int sys_newcall(int term, int cnt, const char __user *option) {
	int ret = 0, zeroCnt = 0, startLoc = 0, startVal = 0, i;
	char buff[4];
	int opt[4];

	//valid check
	if(term < 1 || term > 100 || cnt < 1 || cnt > 100)	return -1;
	
	//get option from user
	copy_from_user(buff, option, 4);
	opt[0] = buff[0] - '0';	opt[1] = buff[1] - '0';
	opt[2] = buff[2] - '0';	opt[3] = buff[3] - '0';
	
	for(i = 3; i >= 0; i--){
		//valid check
		if(opt[i] < 0 || opt[i] > 8)	return -1;
		if(opt[i] == 0)	zeroCnt++;
		else{
			startLoc = i;
			startVal = opt[i];
		} 	
	}
	//valid check
	if(zeroCnt != 3)	return -1;

	//set 4 byte stream
	ret |= (startLoc << 24);
	ret |= (startVal << 16);
	ret |= (term << 8);
	ret |= cnt;

	return ret;
}
