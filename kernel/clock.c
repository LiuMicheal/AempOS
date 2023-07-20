
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               clock.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "cpu.h"	//deleted by mingxuan 2019-1-22


//extern volatile u32 *lapic;	//added by mingxuan 2019-1-26

/*======================================================================*
                    clock_handler(多核情形下为本地时钟中断处理程序)
 *======================================================================*/
PUBLIC void clock_handler(int irq)
{
	/* //deleted by mingxuan 2019-5-7
	acquire(&tickslock);
	ticks++;	//ticks++转移到外部时钟,这样就可以避免使用互斥机制
	release(&tickslock);
	*/

	/*
	disp_str(" Clock-");
	disp_int(cpu->id);
	disp_str("-");
	disp_int(irq);
	disp_str(" ");
	*/

	/* There is two stages - in kernel intializing or in process running.
	 * Some operation shouldn't be valid in kernel intializing stage.
	 * added by xw, 18/6/1
	 */
	if(kernel_initial == 1){
		return;
	}
	
	//p_proc_current->task.ticks--;
	proc->task.ticks--;		//deleted by mingxuan 2019-1-24

	sys_wakeup(&ticks);		//deleted by mingxuan 2019-1-24

	//to make syscall reenter, deleted by xw, 17/12/11
	/*
	if (k_reenter != 0) {
		return;
	}
	*/
 
	/*	//Moved into schedule(). xw, 18/4/21
	if (p_proc_current->task.ticks > 0) {
		//do you know why this statement is added here? p_proc_current doesn't change if schedule() below 
		//isn't called. if you know the reason, please contact me at dongxuwei@163.com. added by xw, 17/12/16
//		cr3_ready = p_proc_current->task.cr3;  //add by visual 2016.5.26		
		return;
	}
	*/

//	schedule();
//	cr3_ready = p_proc_current->task.cr3;			//add by visual 2016.4.5
//	sched();
}

/*======================================================================*
                io_clock_handler(多核情形下为外部时钟中断处理程序)
	added by mingxuan 2019-5-7
 *======================================================================*/
//外部时钟的中断处理程序每次仅有一颗cpu可以执行
PUBLIC void io_clock_handler(int irq)
{
	//disp_str(" io_clock!");
	ticks++; //modified by mingxuan 2019-5-7
}


/*======================================================================*
                              milli_delay
 *======================================================================*/
PUBLIC void milli_delay(int milli_sec)
{
        int t = get_ticks();

        while(((get_ticks() - t) * 1000 / HZ) < milli_sec) {}
}

/*======================================================================*
                init_clock	added by mingxuan 2019-3-10
 *======================================================================*/
PUBLIC void init_clock()
{
	/* initialize 8253 PIT */
    out_byte(TIMER_MODE, RATE_GENERATOR);	//deleted by mingxuan 2019-1-16
    out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );	//deleted by mingxuan 2019-1-16
    out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));	//deleted by mingxuan 2019-1-16
	
	/* initialize clock-irq */
	//初始化本地时钟
    put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */	
    enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */	

	//初始化外部时钟
	//added by mingxuan 2019-5-7
	put_irq_handler(IO_CLOCK_IRQ, io_clock_handler); 								  		
	ioapicenable(IO_CLOCK_IRQ, CPU0_ID);	//打开ioapic的总线时钟的中断引脚(2#),设置cpu0接收外部时钟
}