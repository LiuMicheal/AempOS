
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* EXTERN is defined as extern except in global.c */

/* equal to 1 if kernel is initializing, equal to 0 if done.
 * added by xw, 18/5/31
 */
EXTERN	int		    kernel_initial;

EXTERN	int		    ticks;

EXTERN	int		    disp_pos;

EXTERN	u8		    gdt_ptr[6];	// 0~15:Limit  16~47:Base
EXTERN	DESCRIPTOR	gdt[GDT_SIZE];
EXTERN	u8		    idt_ptr[6];	// 0~15:Limit  16~47:Base
EXTERN	GATE		idt[IDT_SIZE];

EXTERN	u32		    k_reenter;
EXTERN  int         u_proc_sum; 		//内核中用户进程/线程数量 add by visual 2016.5.25

EXTERN	TSS		    tss;

//EXTERN	PROCESS*	p_proc_current; //deleted by mingxuan 2019-1-16
EXTERN	PROCESS*	p_proc_next;    //the next process that will run. added by xw, 18/4/26
                                    

extern	PROCESS		cpu_table[];	    //added by xw, 18/6/1
extern	PROCESS		proc_table[];

//extern	char		task_stack[];   //deleted by mingxuan 2019-1-20
extern  TASK        task_table[];
extern	irq_handler	irq_table[];


//EXTERN	u32 PageTblNum;		//页表数量		add by visual 2016.4.5
//EXTERN	u32 cr3_ready;		//当前进程的页目录		add by visual 2016.4.5  //deleted by mingxuan 2019-1-22

struct memfree{
	u32	addr;
	u32	size;
};

extern struct spinlock ptlock;  //proc_table锁  //added by mingxuan 2019-1-16
extern struct spinlock cr3lock; //cr3_ready锁	 //added by mingxuan 2019-1-21

extern struct spinlock allocPCBlock; //对alloc_PCB这一公共过程加锁	//added by mingxuan 2019-3-5

/* 和文件系统有关的锁 */
extern struct spinlock inode_table_lock;  //added by mingxuan 2019-3-20
extern struct spinlock f_desc_table_lock; //added by mingxuan 2019-3-20
extern struct spinlock super_block_lock;	//added by mingxuan 2019-3-21
extern struct spinlock alloc_imap_bit_lock; //added by mingxuan 2019-3-20
extern struct spinlock alloc_smap_bit_lock; //added by mingxuan 2019-3-20
extern struct spinlock sync_inode_lock;	//added by mingxuan 2019-3-25

extern struct spinlock load_balance_lock;	//added by mingxuan 2019-4-3

/*
extern struct spinlock  execlock;   //added by mingxuan 2019-3-7
extern struct spinlock  forklock;   //added by mingxuan 2019-3-12
extern struct spinlock  pthreadlock; //added by mingxuan 2019-3-14
*/

/* 信号量 */
extern struct semaphore inode_table_sem; //added by mingxuan 2019-3-28