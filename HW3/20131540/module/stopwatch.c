#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <asm/gpio.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/cdev.h>

/*
		device file name : /dev/stopwatch
		major number : 242
*/

///////////////////////////////fpga_fnd///////////////////////////////////////
#define IOM_FND_ADDRESS 0x08000004 // pysical address
static unsigned char *iom_fpga_fnd_addr;
ssize_t iom_fnd_write(int min, int sec);
//////////////////////////////////////////////////////////////////////////////

static int stopwatch_major = 242, stopwatch_minor = 0;
static int result;
static dev_t stopwatch_dev;
static struct cdev stopwatch_cdev;
static int stopwatch_open(struct inode *, struct file *);
static int stopwatch_release(struct inode *, struct file *);
static int stopwatch_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler3(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg);

static stopwatch_usage = 0;

//declare and init statically
DECLARE_WAIT_QUEUE_HEAD(waitQueue);

static struct file_operations stopwatch_fops =
{
	.open = stopwatch_open,
	.write = stopwatch_write,
	.release = stopwatch_release,
};

////////////////////////////////////timer/////////////////////////////////////

static struct struct_mydata
{
	struct timer_list timer;
	int min;
	int sec;
	int pflag;
	unsigned long term;
};
struct struct_mydata mydata;
static struct timer_list exit_timer;
void kernel_timer_func(unsigned long timeout);
void kernel_exit_timer_func(unsigned long timeout);
ssize_t kernel_timer_write(void);
static int startFlag;
//////////////////////////////////////////////////////////////////////////////

/*
	Interrupt Handler
	Home
	Start the timer
*/
irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "interrupt1!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 11)));
	//__wake_up(&waitQueue, 1, 1, NULL);
	//start timer if not paused
	if(!startFlag){
		kernel_timer_write();
		startFlag = 1;
	}
	return IRQ_HANDLED;
}

/*
	Interrupt Handler
	Back
	Pause the timer
*/
irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg) {
    printk(KERN_ALERT "interrupt2!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 12)));
		//change term	if not paused
		if(!mydata.pflag){
			mydata.term = mydata.timer.expires - get_jiffies_64();
			del_timer(&mydata.timer);	//delete timer
			mydata.pflag = 1;
		}
		else{	//restart timer
			kernel_timer_write();
			mydata.pflag = 0;
		}
   	return IRQ_HANDLED;
}

/*
	Interrupt Handler
	VOL+
	Reset the timer
*/
irqreturn_t inter_handler3(int irq, void* dev_id,struct pt_regs* reg) {
    printk(KERN_ALERT "interrupt3!!! = %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));
		mydata.min = mydata.sec = 0;
		mydata.term = HZ;
		iom_fnd_write(0, 0);	//clean up fnd
		kernel_timer_write();	//reset timer
		return IRQ_HANDLED;
}

/*
	Interrupt Handler
	VOL-
	Terminate the timer
*/
irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg) {
		
		int val = gpio_get_value(IMX_GPIO_NR(5, 14));
    
		printk(KERN_ALERT "interrupt4!!! = %x\n", val);
		//Falling
  	if(val == 0){
			//register exit_tiemr
			exit_timer.expires = get_jiffies_64() + 3 * HZ;	//3 sec
    	exit_timer.data = (unsigned long)&exit_timer;
   		exit_timer.function = kernel_exit_timer_func;
	
    	//add timer
    	add_timer(&exit_timer);
		}
		//Rising
		else{
			//if not expired
			del_timer(&exit_timer);
		}

		//
	
		//
    return IRQ_HANDLED;
}

/*
	This is called whenever a process attempts to open the device file
	register interrupt handlers
*/
static int stopwatch_open(struct inode *minode, struct file *mfile){
	int ret;
	int irq;

	printk(KERN_ALERT "Open Module\n");
	
	// int1
	//home
	gpio_direction_input(IMX_GPIO_NR(1,11));
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler1, IRQF_TRIGGER_RISING, "interrupt", NULL);

	// int2
	//back
	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler2, IRQF_TRIGGER_RISING, "interrupt", NULL);

	// int3
	//vol+
	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler3, IRQF_TRIGGER_RISING, "interrupt", NULL);

	// int4
	//vol-
	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler4, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "interrupt", NULL);
	
	startFlag = 0;

	mydata.min = 0;
	mydata.sec = 0;
	mydata.pflag = 0;
	mydata.term = HZ;		//1 sec

	return 0;
}

/*
	This is called when a process attempts to close the device file
	free interrupt handlers
*/
static int stopwatch_release(struct inode *minode, struct file *mfile){
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);
	startFlag = 0;
	printk(KERN_ALERT "Release Module\n");
	return 0;
}

//write data to device using iomapping
ssize_t iom_fnd_write(int min, int sec)
{
    unsigned char value[4]; //4 byte
    unsigned short int value_short = 0;

    //set value[]
    value[0] = min / 10;	value[1] = min % 10;
		value[2] = sec / 10;  value[3] = sec % 10;
    
    //set value_short
    value_short = value[0] << 12 | value[1] << 8 | value[2] << 4 | value[3];

    //write value_short to device
    outw(value_short, (unsigned int)iom_fpga_fnd_addr);
    return 4;
}

//timer callback function
//called at exit timer interrupt
void kernel_exit_timer_func(unsigned long timeout)
{
		del_timer(&exit_timer);
		del_timer(&mydata.timer);	//delete timer
		iom_fnd_write(0, 0);	//clean up fnd 
		__wake_up(&waitQueue, 1, 1, NULL);	//app will close the device file
}

//timer callback function
//called at every timer interrupt
void kernel_timer_func(unsigned long timeout)
{
		struct struct_mydata *p_data = (struct stuct_mydata *)timeout;
		int min = p_data->min, sec = p_data->sec;

		

		sec++;
		if(sec == 60){
			sec = 0;
			min++;
			if(min == 60){
				min = 0;
			}
		}
		
		//change devices state
		iom_fnd_write(min, sec);
		printk("[Timer]\tmin : %d sec : %d term : %ld\n", min, sec, p_data->term);
		p_data->min= min;	p_data->sec = sec;	p_data->term = HZ;

    //update timer
    mydata.timer.expires = get_jiffies_64() + mydata.term;	//1 sec
    mydata.timer.data = (unsigned long)&mydata;
    mydata.timer.function = kernel_timer_func;

    //add updated timer
    add_timer(&mydata.timer);
}

//register new timer
ssize_t kernel_timer_write(void)
{
		del_timer(&mydata.timer);
		
    mydata.timer.expires = get_jiffies_64() + mydata.term;
    mydata.timer.data = (unsigned long)&mydata;
    mydata.timer.function = kernel_timer_func;
	
    //add timer
    add_timer(&mydata.timer);

    return 1;
}

/*
	just sleep user process
*/
static int stopwatch_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos ){
	printk("write\n");
	//sleep
	interruptible_sleep_on(&waitQueue);

	return 0;
}

 /*
    Register the character device
		called by init function
 */
static int stopwatch_register_cdev(void)
{
	int error;
	if(stopwatch_major) {
		stopwatch_dev = MKDEV(stopwatch_major, stopwatch_minor);
		error = register_chrdev_region(stopwatch_dev,1,"stopwatch");
	}else{
		error = alloc_chrdev_region(&stopwatch_dev,stopwatch_minor,1,"stopwatch");
		stopwatch_major = MAJOR(stopwatch_dev);
	}
	if(error<0) {
		printk(KERN_WARNING "stopwatch : can't get major %d\n", stopwatch_major);
		return result;
	}
	printk(KERN_ALERT "major number = %d\n", stopwatch_major);
	cdev_init(&stopwatch_cdev, &stopwatch_fops);
	stopwatch_cdev.owner = THIS_MODULE;
	stopwatch_cdev.ops = &stopwatch_fops;
	error = cdev_add(&stopwatch_cdev, stopwatch_dev, 1);
	if(error)
	{
		printk(KERN_NOTICE "inter Register Error %d\n", error);
	}
	return 0;
}

 /*
    initialize the module
 */
static int __init stopwatch_init(void) {
	int result;
	if((result = stopwatch_register_cdev()) < 0 )
		return result;
	printk(KERN_ALERT "Init Module Success \n");
	printk(KERN_ALERT "Device : /dev/stopwatch, Major Num : 242 \n");

	//iomap fpga_fnd
  iom_fpga_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x4);
	
	//init timer
  init_timer(&mydata.timer);
	init_timer(&exit_timer);

	return 0;
}

/*
	clean up module
	Unregister the appropriate file from /proc
*/
static void __exit stopwatch_exit(void) {

	//iounmap fpga_fnd
  iounmap(iom_fpga_fnd_addr);

	//delete timer
  del_timer_sync(&mydata.timer);
	del_timer_sync(&exit_timer);

	cdev_del(&stopwatch_cdev);
	unregister_chrdev_region(stopwatch_dev, 1);
	printk(KERN_ALERT "Remove Module Success \n");
}

module_init(stopwatch_init);
module_exit(stopwatch_exit);
MODULE_LICENSE("GPL");
