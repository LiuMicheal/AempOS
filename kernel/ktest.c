/*
 * To test if new kernel features work normally, and if old features still 
 * work normally with new features added.
 * added by xw, 18/4/27
 */
#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "fs.h"
#include "cpu.h"	  	//added by mingxuan 2019-1-16
//#include "spinlock.h" //added by mingxuan 2019-1-16

/*======================================================================*
                         Interrupt Handling Test
added by xw, 18/4/27
 *======================================================================*/
	/*
void TestA()
{
	int i, j;
	while (1)
	{
		disp_str("A ");

		i = 100;
		while(--i){
			j = 1000;
			while(--j){}
		}
	}
}

void TestB()
{
	int i, j;
	while (1)
	{
		disp_str("B ");

		i = 100;
		while(--i){
			j = 1000;
			while(--j){}
		}
	}
}

void TestC()
{
	int i, j;
	while (1)
	{
		disp_str("C ");

		i = 100;
		while(--i){
			j = 1000;
			while(--j){}
		}
	}
}

void initial()
 {
	int i, j;
	while (1)
	{
		disp_str("I ");

		i = 100;
		while(--i){
			j = 1000;
			while(--j){}
		}
	}
 }
//	*/

/*======================================================================*
                         Exception Handling Test
added by xw, 18/12/19
 *======================================================================*/
	/*
void TestA()
{
	int i, j;
	while (1)
	{
		disp_str("A ");

		i = 100;
		while(--i){
			j = 1000;
			while(--j){}
		}
		
		//generates an Undefined opcode(#UD) exception, without error code
		//UD2 is provided by Intel to explicitly generate an invalid opcode exception.
//		asm volatile ("ud2");
		
		//generates a General Protection(#GP) exception, with error code
		//the privilege level of a procedure must be 0 to execute the HLT instruction
//		asm volatile ("hlt");
		
		//generates a Divide Error(#DE) exception, without error code
		//calculate a / b
		int a, b;
		a = 10, b = 0;
		asm volatile ("mov %0, %%eax\n\t"
					  "div %1\n\t"
					  : 
					  : "r"(a), "r"(b)
					  : "eax");		
		
	}
}

void TestB()
{
	int i, j;
	while (1)
	{
		disp_str("B ");

		i = 100;
		while(--i){
			j = 1000;
			while(--j){}
		}
	}
}

void TestC()
{
	int i, j;
	while (1)
	{
		disp_str("C ");

		i = 100;
		while(--i){
			j = 1000;
			while(--j){}
		}
	}
}

void initial()
 {
	int i, j;
	while (1)
	{
		disp_str("I ");

		i = 100;
		while(--i){
			j = 1000;
			while(--j){}
		}
	}
 }
//	*/

/*======================================================================*
                        Ordinary System Call Test
added by xw, 18/4/27
 *======================================================================*/
	/*
void TestA()
{
	int i, j;
	while (1)
	{
		disp_str("A ");
		milli_delay(100);
	}
}

void TestB()
{
	int i, j;
	while (1)
	{
		disp_str("B ");
		milli_delay(100);
	}
}

void TestC()
{
	int i, j;
	while (1)
	{
		disp_str("C ");
		milli_delay(100);
	}
}

void initial()
 {
	int i, j;
	while (1)
	{
		disp_str("I ");
		milli_delay(100);
	}
 }
//	*/
	 

/*======================================================================*
                          Kernel Preemption Test
added by xw, 18/4/27
 *======================================================================*/
	/*
void TestA()
{
	int i, j;
	while (1)
	{
		disp_str("A ");
		print_E();
		milli_delay(100);
	}
}

void TestB()
{
	int i, j;
	while (1)
	{
		disp_str("B ");
		print_F();
		milli_delay(100);
	}
}

void TestC()
{
	int i, j;
	while (1)
	{
		disp_str("C ");
		milli_delay(100);
	}
}

void initial()
 {
	int i, j;
	while (1)
	{
		disp_str("I ");
		milli_delay(100);
	}
 }
//	*/

/*======================================================================*
                          Syscall Yield Test
added by xw, 18/4/27
 *======================================================================*/
	/*
void TestA()
{
	int i;
	while (1) 
	{
		disp_str("A( ");
		yield();
		disp_str(") ");
		milli_delay(100);
	} 
}

void TestB()
{
	int i, j;
	while (1)
	{
		disp_str("B ");
		milli_delay(100);
	}
}

void TestC()
{
	int i, j;
	while (1)
	{
		disp_str("C ");
		milli_delay(100);
	}
}

void initial()
 {
	int i, j;
	while (1)
	{
		disp_str("I ");
		milli_delay(100);
	}
 }
//	*/

/*======================================================================*
                          Syscall Sleep Test
added by xw, 18/4/27
 *======================================================================*/
	/*
void TestA()
{
	int i;
	while (1) 
	{
		disp_str("A( ");
		disp_str("[");
		disp_int(ticks);
		disp_str("] ");
		sleep(5);
		disp_str("[");
		disp_int(ticks);
		disp_str("] ");
		disp_str(") ");
		milli_delay(100);
	} 
}

void TestB()
{
	int i, j;
	while (1)
	{
		disp_str("B ");
		milli_delay(100);
	}
}

void TestC()
{
	int i, j;
	while (1)
	{
		disp_str("C ");
		milli_delay(100);
	}
}

void initial()
 {
	int i, j;
	while (1)
	{
		disp_str("I ");
		milli_delay(100);
	}
 }
//	*/

/*======================================================================*
                          User Process Test
added by xw, 18/4/27
 *======================================================================*/
/* You should also enable the feature you want to test in init.c */
/*
void TestA()
{
	int i, j;
	while (1)
	{
		disp_str("A ");
		yield();
		//milli_delay(100);
	}
}

void TestB()
{
	int i, j;
	while (1)
	{
		disp_str("B ");
		yield();
		//milli_delay(100);
	}
}

void TestC()
{
	int i, j;
	while (1)
	{
		disp_str("C ");
		yield();
		//milli_delay(100);
	}
}


void initial()
 {
	exec("init/init.bin");
	while(1)
	{
		
	}
 }
*/

/*======================================================================*
                          File System Test
added by xw, 18/5/26
 *======================================================================*/
	/*
void TestA()
{	
	//while (1) {}
	
	//manipulate /blah
	//if /blah exists, open
	//if /blah doesn't exist, open+write+close
	int fd;
	int i, n;
	char filename[MAX_FILENAME_LEN+1] = "blah";
	const char bufw[] = "abcde";
	const int rd_bytes = 4;
	char bufr[5];

	disp_str("(TestA)");
	fd = open(filename, O_CREAT | O_RDWR);	
	
	if(fd != -1) {
		disp_str("File created: ");
		disp_str(filename);
		disp_str(" (fd ");
		disp_int(fd);
		disp_str(")\n");	
		
		n = write(fd, bufw, strlen(bufw));
		if(n != strlen(bufw)) {
			disp_str("Write error\n");
		}
		
		close(fd);
	}
	
	while (1) {
	}
}


void TestB()
{	
	//while (1) {}
	
	//manipulate /blah, open+lseek+read+close
	int fd, n;
	const int rd_bytes = 4;
	char bufr[5];
	char filename[MAX_FILENAME_LEN+1] = "blah";

	disp_str("(TestB)");
	fd = open(filename, O_RDWR);
	disp_str("       ");
	disp_str("File opened. fd: ");
	disp_int(fd);
	disp_str("\n");

	disp_str("(TestB)");
	int lseek_status = lseek(fd, 1, SEEK_SET);
	disp_str("Return value of lseek is: ");
	disp_int(lseek_status);
	disp_str("  \n");

	disp_str("(TestB)");
	n = read(fd, bufr, rd_bytes);
	if(n != rd_bytes) {
		disp_str("Read error\n");
	}
	bufr[n] = 0;
	disp_str("Bytes read: ");
	disp_str(bufr);
	disp_str("\n");

	close(fd);

	while(1){

	}
}

void TestC()
{
	//while (1) {}
	
	//manipulate /blah, open+lseek+read+close
	int fd, n;
	const int rd_bytes = 3;
	char bufr[4];
	char filename[MAX_FILENAME_LEN+1] = "blah";
	
	disp_str("(TestC)");
	fd = open(filename, O_RDWR);
	disp_str("       ");
	disp_str("File opened. fd: ");
	disp_int(fd);
	disp_str("\n");

	disp_str("(TestC)");
	int lseek_status = lseek(fd, 1, SEEK_SET);
	disp_str("Return value of lseek is: ");
	disp_int(lseek_status);
	disp_str("  \n");

	disp_str("(TestC)");
	n = read(fd, bufr, rd_bytes);
	if(n != rd_bytes) {
		disp_str("Read error\n");
	}
	bufr[n] = 0;
	disp_str("bytes read: ");
	disp_str(bufr);
	disp_str("\n");

	close(fd);
	
	while(1){
	}
}

void initial()
{
	//while (1) {}
	
	int i, fd;
	char* filenames[] = {"/foo", "/bar", "/baz"};
	char* rfilenames[] = {"/bar", "/foo", "/baz", "/dev_tty0"};
	
	//open+close
	for (i = 0; i < sizeof(filenames) / sizeof(filenames[0]); i++) {
		disp_str("(Initial)");
		fd = open(filenames[i], O_CREAT | O_RDWR);
		if(fd != -1) {
			disp_str("File created: ");
			disp_str(filenames[i]);
			disp_str(" (fd ");
			disp_int(fd);
			disp_str(")\n");

			close(fd);
		}
	}

	//unlink
	for (i = 0; i < sizeof(rfilenames) / sizeof(rfilenames[0]); i++) {
		disp_str("(Initial)");
		if (unlink(rfilenames[i]) == 0) {
			disp_str("File removed: ");
			disp_str(rfilenames[i]);
			disp_str("\n");
		}
		else {
			disp_str("         ");
			disp_str("Failed to remove file: ");
			disp_str(rfilenames[i]);
			disp_str("\n");
		}
	}
	
	while (1){
	}
}
//	*/

/*======================================================================*
                          File System Test 2
to test interrupt_wait_sched() used in hd.c
interrupt_wait_sched() is not used currently, because it can only support
one process to access hd disk, as you can see below.
added by xw, 18/8/16
 *======================================================================*/
	/*
void TestA()
{
	int i, j;
	while (1)
	{
		disp_str("A ");
		milli_delay(100);
	}
}

void TestB()
{	
	//while (1) {}
	
	int fd, n;
	char bufr[5];
	char bufw[5] = "mmnn";
	char filename[MAX_FILENAME_LEN+1] = "blah";

	disp_str("(TestB)");
	fd = open(filename, O_RDWR);
	disp_str("       ");
	disp_str("File opened. fd: ");
	disp_int(fd);
	disp_str("\n");

	disp_str("(TestB)");
	lseek(fd, 1, SEEK_SET);
	n = write(fd, bufw, 4);
	if(n != 4) {
		disp_str("Write error\n");
	}
	bufr[4] = 0;
	disp_str("Bytes write: ");
	disp_str(bufw);
	disp_str("\n");
	
	disp_str("(TestB)");
	lseek(fd, 1, SEEK_SET);
	n = read(fd, bufr, 4);
	if(n != 4) {
		disp_str("Read error\n");
	}
	bufr[4] = 0;
	disp_str("Bytes read: ");
	disp_str(bufr);
	disp_str("\n");

	close(fd);

	while(1){

	}
}

void TestC()
{
	int i, j;
	while (1)
	{
		disp_str("C ");
		milli_delay(100);
	}
}

void initial()
 {
	int i, j;
	while (1)
	{
		disp_str("I ");
		milli_delay(100);
	}
 }
//	*/


/*======================================================================*
                MP Processes Schedule Test by SYS_CALL
added by mingxuan 2019-1-9
 *======================================================================*/
/*
void TestA()
{
	int i, j;
	//struct cpu *c = 0;;

	while (1)
	{
		//acquire(&TestAlock); //added by mingxuan 2019-1-16
		disp_str("A-");
		disp_int(cpu->id);
		disp_str(" ");

		for(i=10000000; i>0; i--);

		//yield();	//deleted by mingxuan 2019-1-26
		//milli_delay(100);

		//release(&TestAlock); //added by mingxuan 2019-1-16
	}
}

void TestB()
{
	int i, j;
	int *p=0xFEE00390;

	while (1)
	{
		disp_str("B-");
		disp_int(cpu->id);
		disp_str(" ");

		for(i=10000000; i>0; i--);	

		//yield();	//deleted by mingxuan 2019-1-26
		//milli_delay(100);
	}
}

void TestC()
{
	int i, j;
	while (1)
	{
		//acquire(&TestClock); //added by mingxuan 2019-1-16

		disp_str("C-");
		disp_int(cpu->id);
		disp_str(" ");

		for(i=10000000; i>0; i--);

		//yield();	//deleted by mingxuan 2019-1-26
		//milli_delay(100);

		//release(&TestClock); //added by mingxuan 2019-1-16
	}
}

void TestD()
{
	int i, j;
	int *p=0xFEE00390;	//lapic[TCCR]
						//硬件原理：TCCR的值不断减少，当减到0时，又恢复到TICR设定的值，继续减少
	while (1)
	{
		//acquire(&TestDlock); //added by mingxuan 2019-1-16

		disp_str("D-");
		disp_int(cpu->id);
		disp_str(" ");
		
		for(i=10000000; i>0; i--);

		//yield();	//deleted by mingxuan 2019-1-26
		//milli_delay(100);

		//release(&TestDlock); //added by mingxuan 2019-1-16
	}
}

void TestE()
{
	int i, j;
	while (1)
	{
		//acquire(&TestElock); //added by mingxuan 2019-1-16

		disp_str("E-");
		disp_int(cpu->id);
		disp_str(" ");

		for(i=10000000; i>0; i--);

		//yield();	//deleted by mingxuan 2019-1-26
		//milli_delay(100);

		//release(&TestElock); //added by mingxuan 2019-1-16
	}
}

void TestF()
{
	int i, j;
	int *p=0xFEE00390;
	while (1)
	{
		//acquire(&TestFlock); //added by mingxuan 2019-1-16

		disp_str("F-");
		disp_int(cpu->id);
		disp_str(" ");

		for(i=10000000; i>0; i--);

		//yield();	//deleted by mingxuan 2019-1-26
		//milli_delay(100);

		//release(&TestFlock); //added by mingxuan 2019-1-16
	}
}
*/

/*======================================================================*
                MP Processes Schedule Test by initial
added by mingxuan 2019-1-26
 *======================================================================*/
/*
void TestA()
{
	int i, j;

	while (1)
	{
		//acquire(&TestAlock); //added by mingxuan 2019-1-16
	
		
		disp_str("A-");
		disp_int(cpu->id);
		disp_str(" ");
		

		for(i=10000000; i>0; i--);

		//yield();	//deleted by mingxuan 2019-1-26
		//milli_delay(100);

		//release(&TestAlock); //added by mingxuan 2019-1-16
	}
}

void TestB()
{
	int i, j;

	while (1)
	{
		//acquire(&TestAlock); //added by mingxuan 2019-1-16

		
		disp_str("B-");
		disp_int(cpu->id);
		disp_str(" ");
		

		for(i=10000000; i>0; i--);

		//yield();	//deleted by mingxuan 2019-1-26
		//milli_delay(100);
	}
}

void TestC()
{
	int i, j;

	while (1)
	{
		//acquire(&TestClock); //added by mingxuan 2019-1-16

		
		disp_str("C-");
		disp_int(cpu->id);
		disp_str(" ");
		

		for(i=10000000; i>0; i--);

		//yield();	//deleted by mingxuan 2019-1-26
		//milli_delay(100);

		//release(&TestClock); //added by mingxuan 2019-1-16
	}
}

void TestD()
{
	int i, j;

	while (1)
	{
		//acquire(&TestDlock); //added by mingxuan 2019-1-16

		
		disp_str("D-");
		disp_int(cpu->id);
		disp_str(" ");
		

		for(i=10000000; i>0; i--) ;

		//yield();	//deleted by mingxuan 2019-1-26
		//milli_delay(100);

		//release(&TestDlock); //added by mingxuan 2019-1-16
	}
}

void initial()
 {
	int i, j;
	exec("init/init.bin");
 }
*/

/*======================================================================*
                          User Process Test
added by xw, 18/4/27
 *======================================================================*/
/* You should also enable the feature you want to test in init.c */
//	/*
void TestA()
{
	int i, j;
	while (1)
	{
		//disp_str("A ");
		for(i=10000000; i>0; i--) ;
		//milli_delay(100);
	}
}

void TestB()
{
	int i, j;
	while (1)
	{
		//disp_str("B ");
		for(i=10000000; i>0; i--) ;
		//milli_delay(100);
	}
}

//deleted by mingxuan 2019-3-8
/*
void TestC()
{
	int i, j;
	while (1)
	{
		disp_str("C-");
		disp_int(cpu->id);
		disp_str(" ");
		for(i=10000000; i>0; i--) ;
	}
}
*/

//deleted by mingxuan 2019-3-6
/*
void TestD()
{
	int i, j;
	while (1)
	{
		disp_str("D-");
		disp_int(cpu->id);
		disp_str(" ");
		for(i=10000000; i>0; i--) ;
	}
}
*/

/*
//cpu0的initial系统进程
void initial()
 {
	int i;
	//exec("init/init.bin");
	while(1)
	{
		disp_str("I0 ");
		for(i=10000000; i>0; i--) ;
	}
 }

//cpu1的initial1系统进程
 void initial1()
 {
	int i;
	//exec("init/init1.bin");
	while(1)
	{
		disp_str("I1 ");
		for(i=10000000; i>0; i--) ;
	}
 }

 //cpu2的initial1系统进程
 void initial2()
 {
	int i;
	//exec("init/init2.bin");
	while(1)
	{
		disp_str("I2 ");
		for(i=10000000; i>0; i--) ;
	}
 }

 //cpu3的initial3系统进程
 void initial3()
 {
	int i;
	//exec("init/init3.bin");
	while(1)
	{
		disp_str("I3 ");
		for(i=10000000; i>0; i--) ;
	}
 }
*/

//由initial1/2/3执行
//延时，等待创建init0完成后，init1/2/3才执行其进程体
//added by mingxuan 2019-4-12
void wait_init0()
{
	int child_pid = 0;

	//获取子进程的pid
	child_pid = proc->task.info.child_process[proc->task.info.child_p_num-1];

	//在当前cpu就绪队列中搜索，将其状态暂时置为SLEEPING，等待init0初始化完成
	struct ready_proc *p_mov = cpu->ready.front;
	while(p_mov != NULL)
    {
		if(p_mov->proc->task.pid == child_pid)
		{
			p_mov->proc->task.stat = SLEEPING; //fork结束后，子进程就已经被挂载到cpu的就绪队列上，要想其不参与调度，只需将其睡眠
		}	
		p_mov = p_mov->next;
    }		
}

//由initial执行
//唤醒等待的init1/2/3
void sync_init()
{
	struct cpu *c = NULL;
	PROCESS *p = NULL;
	int child_pid = 0;

	//遍历cpu1/2/3的就绪队列，将其睡眠的init1/2/3的状态修改为READY
	for(c = cpulist+1, p = proc_table+1; c < cpulist+cpunumber, p < proc_table+NR_INITIALS; c++, p++)
	{
		child_pid = p->task.info.child_process[p->task.info.child_p_num-1]; //获取子进程的pid
		
		struct ready_proc *p_mov = c->ready.front;
		while(p_mov != NULL)
    	{
			if(p_mov->proc->task.pid == child_pid)
			{
				p_mov->proc->task.stat = READY;
			}	
			p_mov = p_mov->next;
    	}	
	}
}

//唤醒所有的migration内核线程
//仅有init执行，因为init是最后执行的
//added by mingxuan 2019-4-11
void wakeup_migration()
{
	struct cpu *c;
	PROCESS *p;

	//遍历cpu0/1/2/3的就绪队列，将IDLE状态的migration0/1/2/3的状态修改为READY
	for(c = cpulist, p = proc_table + NR_INITIALS; c < cpulist+cpunumber, p < proc_table+NR_INITIALS+NR_MIGRATIONS; c++, p++)
	{	
		struct ready_proc *p_mov = c->ready.front;
		while(p_mov != NULL)
    	{	
			switch (c->id)
			{
				case 0: if(p_mov->proc->task.pid == MIGRATION_PID)  p_mov->proc->task.stat = READY; break;
				case 1: if(p_mov->proc->task.pid == MIGRATION1_PID) p_mov->proc->task.stat = READY; break;
				case 2: if(p_mov->proc->task.pid == MIGRATION2_PID) p_mov->proc->task.stat = READY; break;
				case 3: if(p_mov->proc->task.pid == MIGRATION3_PID) p_mov->proc->task.stat = READY; break;			
				default: break;
			}
			p_mov = p_mov->next;
    	}	
	}
}

//保证init是在cpu0的就绪队列的第三个位置
//此时队尾一定是init，该函数把队尾的init放至cpu0的就绪队列的第三个位置
void resort_ready_proc()
{
	struct cpu *c = cpu; //仅用于调试
	struct ready_proc *p_mov = cpu->ready.front->next; //p_mov指向就绪队列的第2个节点
	struct ready_proc *p_init = cpu->ready.rear; //此时的init节点在最后

	cpu->ready.rear = cpu->ready.rear->prev;

	p_init->next = p_mov->next;
	p_init->prev = p_mov;
	p_mov->next->prev = p_init;
	p_mov->next = p_init;

	cpu->ready.rear->next = NULL;
}

//保证init是在cpu0的就绪队列的第三个位置
//cpu0的initial系统进程
void initial()
{
	int pid, i;

	pid = fork();
	if (pid == 0)
	{//子进程(相当于1号进程)

		sync_init(); //依次唤醒等待的init1/2/3，实现同步
		wakeup_migration(); //唤醒所有的migration内核线程

		exec("init/init.bin"); //进入exec后程序就不会再返回
		//此处永远不会被执行
	} 
	else
	{//父进程(相当于0号进程/IDLE进程)
		while(1)
		{
			resort_ready_proc(); //保证init是在cpu0的就绪队列的第三个位置

			//disp_str("I0 ");
			for(i=10000000; i>0; i--) ;
		}
	}
}

//cpu1的initial1系统进程
void initial1()
{
	int pid, i;
	PROCESS *p; //引入此变量仅用于调试

	pid = fork();
	if (pid == 0)
	{//子进程(相当于1号进程)
		exec("init/init1.bin");
	} 
	else
	{//父进程(相当于0号进程/IDLE进程)

		wait_init0(); //延时，等待创建init0完成后，子进程init1才执行其进程体

		while(1)
		{
			//disp_str("I1 ");
			for(i=10000000; i>0; i--) ;
		}
	}
}

 //cpu2的initial1系统进程
void initial2()
{
	int pid, i;

	pid = fork();
	if (pid == 0)
	{//子进程(相当于1号进程)
		exec("init/init2.bin");
	} 
	else
	{//父进程(相当于0号进程/IDLE进程)

		wait_init0(); //延时，等待创建init0完成后，子进程init2才执行其进程体

		while(1)
		{
			//disp_str("I2 ");
			for(i=10000000; i>0; i--) ;
		}
	}
}

//cpu3的initial3系统进程
void initial3()
{
	int pid, i;
	int child_pid;

	pid = fork();
	if (pid == 0)
	{//子进程(相当于1号进程)
		exec("init/init3.bin");
	} 
	else
	{//父进程(相当于0号进程/IDLE进程)

		wait_init0(); //延时，等待创建init0完成后，子进程init3才执行其进程体

		while(1)
		{
			//disp_str("I3 ");
			for(i=10000000; i>0; i--) ;
		}
	}
}


/*======================================================================*
							负载均衡内核线程	
added by mingxuan 2019-4-12
 *======================================================================*/
//added by mingxuan 2019-4-12
void migration()
{
	int i;
	while(1)
	{
		load_balance();	//added by mingxuan 2019-4-12
		//disp_str("M0 ");
		for(i=10000000; i>0; i--) ;
	}
}

//added by mingxuan 2019-4-12
void migration1()
{
	int i;
	while(1)
	{
		load_balance();	//added by mingxuan 2019-4-12
		//disp_str("M1 ");
		for(i=10000000; i>0; i--) ;
	}
}

//added by mingxuan 2019-4-12
void migration2()
{
	int i;
	while(1)
	{
		load_balance();	//added by mingxuan 2019-4-12
		//disp_str("M2 ");
		for(i=10000000; i>0; i--) ;
	}
}

//added by mingxuan 2019-4-12
void migration3()
{
	int i;
	while(1)
	{
		load_balance();	//added by mingxuan 2019-4-12
		//disp_str("M3 ");
		for(i=10000000; i>0; i--) ;
	}
}


//用来初始化task_table
void Task()
{
	int i;
	while(1)
	{
		disp_str("Task-");
		disp_int(cpu->id);
		disp_str(" ");
		for(i=10000000; i>0; i--) ;
	}
}
