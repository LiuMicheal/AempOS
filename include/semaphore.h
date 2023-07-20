/**********************************************************
*	semaphore.h       //added by mingxuan 2019-3-28
***********************************************************/

//等待队列的节点
struct wait_proc
{
	PROCESS *proc;
	struct wait_proc *next;
};

//等待队列
struct wait_queue
{
	struct wait_proc *front; //就绪队列的头指针
	struct wait_proc *rear;  //就绪队列的尾指针
};

//信号量
struct semaphore 
{
	int count;				 //共享计数值
	int sleepers;			 //等待当前信号量而进入睡眠的进程个数
	struct wait_queue wait; //当前信号量的等待队列
};

void init_wait_queue(struct wait_queue *wq);
void add_wait_queue(struct wait_queue *wq, struct wait_proc *p);
int remove_wait_queue(struct wait_queue *wq, struct wait_proc **p);

void init_sema(struct semaphore *sem, int val);

static inline void down(struct semaphore *sem)
{
    asm volatile("lock; decl %0\n\t"
                "jns 1f\n"
                "lea %0, %%eax\n\t"
                "call down_failed\n"
                "1:" 
                :"+m" (sem->count)
                :
                :"memory", "ax"
				);    
}

static inline void up(struct semaphore *sem)
{
    asm volatile("lock; incl %0\n\t"
                "jg 2f\n"
                "lea %0, %%eax\n\t"
                "call up_wakeup\n"
                "2:" 
                :"+m" (sem->count)
                :
                :"memory", "ax"
				);    
}