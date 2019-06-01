/*
*   dev_driver.c - create an ouput character device
*/

/////////////////////////Linux Module Programming Guide//////////////////////
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////Given Device Drivers///////////////////////////
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include "./fpga_dot_font.h"
/////////////////////////////////////////////////////////////////////////////

#define SUCCESS 0
#define DEVICE_NAME "dev_driver"

////////////////////////////Same with app/////////////////////////////////////
#define MAJOR_NUM 242
#define GET_DATA 1 //not use in here
#define PUT_DATA 2
#define IOCTL_SET_NUM _IOW(MAJOR_NUM, PUT_DATA, int *)
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////fpga_fnd///////////////////////////////////////
#define IOM_FND_ADDRESS 0x08000004 // pysical address
static unsigned char *iom_fpga_fnd_addr;
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////fpga_led///////////////////////////////////////
#define IOM_LED_ADDRESS 0x08000016 // pysical address
static unsigned char *iom_fpga_led_addr;
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////fpga_dot///////////////////////////////////////
#define IOM_FPGA_DOT_ADDRESS 0x08000210 // pysical address
static unsigned char *iom_fpga_dot_addr;
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////fpga_text_lct//////////////////////////////////
#define IOM_FPGA_TEXT_LCD_ADDRESS 0x08000090 // pysical address - 32 Byte (16 * 2)
static unsigned char *iom_fpga_text_lcd_addr;
//////////////////////////////////////////////////////////////////////////////

////////////////////////////////////timer/////////////////////////////////////
static struct struct_mydata
{
    struct timer_list timer;
    int count;
    int loc;    //fnd
    int val;    //fnd, led, dot
    int idx;     //lcd   (start idx of first, second line)
    int shift;  //0 : to right, 1 : to left
};
struct struct_mydata mydata;
static int endCnt;
static int term;
//////////////////////////////////////////////////////////////////////////////

/****************************      
*        Per a device       *
*                           *
*   1. add global variables *
*   2. add init sequence    *
*   3. add exit sequence    *
*   4. add write function   *
*                           *
*****************************/

static int Device_Open = 0;



static int dev_open(struct inode *inode, struct file *file);
static int dev_release(struct inode *inode, struct file *file);
long dev_ioctl(struct file *inode, unsigned int ioctl_num, unsigned long ioctl_param);
void kernel_timer_func(unsigned long timeout);
ssize_t kernel_timer_write(int loc, int val);
ssize_t iom_fnd_write(int loc, int val);
ssize_t iom_led_write(int val);
ssize_t iom_dot_write(int val);
ssize_t iom_text_lcd_write(int idx);

/* Module Declarations */
/*
* This structure will hold the functions to be called
* when a process does something to the device we
* created. Since a pointer to this structure is kept in
* the devices table, it can't be local to
* init_module. NULL is for unimplemented functions.
*/

struct file_operations Fops = {
    .unlocked_ioctl = dev_ioctl,
    .open = dev_open,
    .release = dev_release,
    /* a.k.a. close */
};

/*
* Initialize the module - Register the character device
*/
int __init dev_init(void)
{
    int ret_val;
    /*
    * Register the character device (atleast try)
    */
    ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);

    /*
    * Negative values signify an error
    */
    if (ret_val < 0)
    {
        printk(KERN_ALERT "%s failed with %d\n",
               "Sorry, registering the character device ", ret_val);
        return ret_val;
    }
    printk(KERN_INFO "%s The major device number is %d.\n",
           "Registeration is a success", MAJOR_NUM);
    printk(KERN_INFO "If you want to talk to the device driver,\n");
    printk(KERN_INFO "you'll have to create a device file. \n");
    printk(KERN_INFO "We suggest you use:\n");
    printk(KERN_INFO "mknod %s c %d 0\n", DEVICE_NAME, MAJOR_NUM);
    printk(KERN_INFO "The device file name is important, because\n");
    printk(KERN_INFO "the ioctl program assumes that's the\n");
    printk(KERN_INFO "file you'll use.\n");

    //iomap fpga_fnd
    iom_fpga_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x4);
    //iomap fpga_led
    iom_fpga_led_addr = ioremap(IOM_LED_ADDRESS, 0x1);
    //iomap fpga_dot
    iom_fpga_dot_addr = ioremap(IOM_FPGA_DOT_ADDRESS, 0x10);
    //iomap fpga_text_lcd
    iom_fpga_text_lcd_addr = ioremap(IOM_FPGA_TEXT_LCD_ADDRESS, 0x32);

    //init timer
    init_timer(&mydata.timer);
    return 0;
}

/*
* Cleanup - unregister the appropriate file from /proc
*/
void __exit dev_exit(void)
{
    // int ret;

    //iounmap fpga_fnd
    iounmap(iom_fpga_fnd_addr);
    //iounmap fpga_led
    iounmap(iom_fpga_led_addr);
    //ioummap fpga_dot
    iounmap(iom_fpga_dot_addr);
    //ioummap fpga_text_lct
    iounmap(iom_fpga_text_lcd_addr);

    //delete timer
    del_timer_sync(&mydata.timer);

    /*
    * Unregister the device
    */
    //ret = unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);

    printk(KERN_INFO "%s The major device number is %d.\n",
           "Unegisteration is a success", MAJOR_NUM);
           

    /*
    * If there's an error, report it
    */
    //if (ret < 0)
    //   printk(KERN_ALERT "Error: unregister_chrdev: %d\n", ret);
}

/*
* This is called whenever a process attempts to open the device file
*/
static int dev_open(struct inode *inode, struct file *file)
{

    /*
    * We don't want to talk to two processes at the same time
    */
    if (Device_Open)
        return -EBUSY;
    Device_Open++;
    /*
    * Initialize the message
    */

    try_module_get(THIS_MODULE);
    return SUCCESS;
}

static int dev_release(struct inode *inode, struct file *file)
{
    /*
    * We're now ready for our next caller
    */
    Device_Open--;
    module_put(THIS_MODULE);

    return SUCCESS;
}

//timer callback function
//called at every timer interrupt
void kernel_timer_func(unsigned long timeout)
{
    struct struct_mydata *p_data = (struct struct_mydata *)timeout;
    int loc = p_data->loc, val = p_data->val, idx = p_data->idx, shift = p_data->shift;

    //increase count
    p_data->count++;

    //terminate condition
    if (p_data->count > endCnt)
    {

        //turn off every device here!!
        iom_fnd_write(0, 0);

        iom_led_write(0);

        iom_dot_write(-1);

        iom_text_lcd_write(-1);

        return;
    }

    //change loc, val
    val++;
    if (val == 9)
    {
        val = 1;
        loc++;
        if (loc == 4)
            loc = 0;
    }

    //change idx, shift
    if(!shift){
        idx++;
        if(idx == 9){
            idx = 7;
            shift = !shift; 
        }
    }
    else{
        idx--;
        if(idx == -1){
            idx = 2;
            shift = !shift;
        }
    }

    printk("IN Timer : \n loc : %d val : %d idx : %d shift : %d\n", loc, val, idx, shift);

    p_data->loc = loc;
    p_data->val = val;
    p_data->idx = idx;
    p_data->shift = shift;

    //change devices state
    iom_fnd_write(loc, val);

    iom_led_write(val);

    iom_dot_write(val);

    iom_text_lcd_write(idx);

    //update timer
    mydata.timer.expires = get_jiffies_64() + (term * HZ) / 10;
    mydata.timer.data = (unsigned long)&mydata;
    mydata.timer.function = kernel_timer_func;

    //add updated timer
    add_timer(&mydata.timer);
}

ssize_t kernel_timer_write(int loc, int val)
{

    //init timer info
    mydata.count = 0;
    mydata.loc = loc;
    mydata.val = val;
    mydata.idx = 0;
    mydata.shift = 0;

    printk("IN Timer : \n loc : %d val : %d idx : %d shift : %d\n", loc, val, 0, 0);
    del_timer_sync(&mydata.timer);

    mydata.timer.expires = get_jiffies_64() + (term * HZ) / 10;
    mydata.timer.data = (unsigned long)&mydata;
    mydata.timer.function = kernel_timer_func;

    //add timer
    add_timer(&mydata.timer);

    return 1;
}

//used in device_ioctl

ssize_t iom_fnd_write(int loc, int val)
{
    unsigned char value[4]; //4 byte
    unsigned short int value_short = 0;

    //set value[]
    value[0] = value[1] = value[2] = value[3] = 0;
    value[loc] = val;
    
    //set value_short
    value_short = value[0] << 12 | value[1] << 8 | value[2] << 4 | value[3];

    //write value_short to device
    outw(value_short, (unsigned int)iom_fpga_fnd_addr);
    return 4;
}

ssize_t iom_led_write(int val)
{
    unsigned char value = 0; //8bit
    unsigned short _s_value;

    if(val > 0)
        value |= (1 << (8 - val));

    _s_value = (unsigned short)value;
    outw(_s_value, (unsigned int)iom_fpga_led_addr);

    return 2;
}

ssize_t iom_dot_write(int val)
{
    int i;

    //unsigned char value[10]; //10 byte
    unsigned short int _s_value;

    //use fpga_number in fpga_dot_font.h here

    
    if (val < 0)    
    {
        //clean up
        for (i = 0; i < 10; i++)
        {
            outw(0, (unsigned int)iom_fpga_dot_addr + i * 2);
        }
    }

    else
    {
        for (i = 0; i < 10; i++)
        {
            _s_value = fpga_number[val][i] & 0x7F;
            outw(_s_value, (unsigned int)iom_fpga_dot_addr + i * 2);
        }
    }
    return 10;
}

ssize_t iom_text_lcd_write(int idx)
{ //may need some params
    
        int i, j;
        unsigned char str1[8] = "20131540";
        unsigned char str2[8] = "KimJunho";

        unsigned short int _s_value = 0;
        unsigned char value[33]; //32byte
        //const char *tmp = gdata;

        value[32] = 0;
        //	printk("Get Size : %d / String : %s\n",length,value);
        if(idx < 0)
        {
            //clean up
            for (i = 0; i < 32; i++)
           {
             outw(0, (unsigned int)iom_fpga_text_lcd_addr + i);
             i++;
           }
        }
        else
        {   
           memset(value, 0, sizeof(value));
           for(i = idx, j = 0; i < idx + 8; i++, j++)
           {
               value[i] = str1[j];
               value[i + 16] = str2[j];
           }
           
           for (i = 0; i < 32; i++)
           {
             _s_value = (value[i] & 0xFF) << 8 | value[i + 1] & 0xFF;
             outw(_s_value, (unsigned int)iom_fpga_text_lcd_addr + i);
             i++;
           }
        }

        return 32;
    
}

/*
* This function is called whenever a process tries to do an ioctl on our
* device file. We get two extra parameters (additional to the inode and file
* structures, which all device functions get): the number of the ioctl called
* and the parameter given to the ioctl function.
*
* If the ioctl is write or read/write (meaning output is returned to the
* calling process), the ioctl call returns the output of this function.
*
*/
long dev_ioctl(
		/* see include/linux/fs.h */
		struct file *inode,
		/* ditto */
		unsigned int ioctl_num,
		/* number and param for ioctl */
		unsigned long ioctl_param)
{
	int *temp;
	int sys_res, bitmask = 0x000000FF;
	int loc = 0, val = 0;

	/*
	 * Switch according to the ioctl called
	 */
    
	switch (ioctl_num) //only a single command in HW2
	{
		case IOCTL_SET_NUM:

			temp = (int *)ioctl_param;

			get_user(sys_res, temp);

			/***********************************************************************************
				sys_res : 

				4 byte stream (integer)

				|------------- in mydata ------------|----------- global ----------|      //in dev_driver.c
				 __________________________________________________________________
				|start loc of fnd | start val of fnd | time term     | # of print  |         
				|_________________|__________________|_______________|_____________|
				4                 3                  2               1             0
				(byte offset)

			 ************************************************************************************/

			//decode from system call result

            printk("sys_res in driver : %d\n", sys_res);

			endCnt = (sys_res & bitmask);
			bitmask <<= 8;
			term = (sys_res & bitmask);
			term >>= 8;
			bitmask <<= 8;
			val = (sys_res & bitmask);
			val >>= 16;
			bitmask <<= 8;
			loc = (sys_res & bitmask);
			loc >>= 24;

			printk("loc : %d val : %d, term : %d, # : %d\n", loc, val, term, endCnt);

			iom_fnd_write(loc, val);

			iom_led_write(val);

			iom_dot_write(val);

			iom_text_lcd_write(0);

			kernel_timer_write(loc, val);

			break;

			//other cases..
	}
	return SUCCESS;
}

module_init(dev_init);
module_exit(dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huins");
