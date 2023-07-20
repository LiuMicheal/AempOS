/**********************************************************
*	semaphore.c       //added by mingxuan 2019-3-28
***********************************************************/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proc.h"
#include "global.h"
#include "cpu.h"
#include "semaphore.h"

int add_negative(int i,int *v);

/*************************************************************************
* 第一部分：等待队列
**************************************************************************/

void init_wait_queue(struct wait_queue *wq)
{
	wq->front = wq->rear = NULL;
}

//从该信号量的等待队列的队尾入队
void add_wait_queue(struct wait_queue *wq, struct wait_proc *p)
{
	p->next = NULL;
	if(wq->rear == NULL) //第一个节点
	{
		//wq->front = wq->rear = p;
		wq->rear = p;
		wq->front = p;
	}
	else
	{
		wq->rear->next = p;
		wq->rear = p;
	}
}

//从该信号量的等待队列的队头出队
int remove_wait_queue(struct wait_queue *wq, struct wait_proc **p)
{
	if (wq->rear == NULL)
		return 0;	//empty

	//*p = wq->front;
	if (wq->front == wq->rear) //put out the last node
	{
		wq->front = wq->rear = NULL;
	} 
	else 
	{
		wq->front = wq->front->next;
	}
	return 1;	//not empty
}


/*************************************************************************
* 第二部分：信号量
**************************************************************************/
void init_sema(struct semaphore *sem, int val)
{
	sem->count = val;
	sem->sleepers = 0;
	init_wait_queue(&sem->wait);
}

void do_down(struct semaphore *sem)
{
	struct wait_proc *wp;
	wp = (struct wait_proc *)K_PHY2LIN(do_kmalloc(sizeof(struct wait_proc)));
	wp->proc = proc;

	add_wait_queue(&sem->wait, wp);
	wp->proc->task.stat = SLEEPING; //修改PCB，此后该进程不再被调度，只有当持有信号量的进程完成互斥区的操作后才将该进程唤醒

	sem->sleepers++;
	while(1)
	{
		int sleepers = sem->sleepers;
		if(!add_negative(sleepers - 1, &sem->count)) //如何表示“sem可用”这个条件?依靠count成员和sleepers成员
		{
			sem->sleepers = 0;
			sem->count = -1;	//added by mingxuan 2019-4-2
			break;
		}
		sem->sleepers = 1;
		sched();	//主动放弃CPU
	}
}

void do_up(struct semaphore *sem)
{
	struct wait_proc *wp;
	wp = sem->wait.front;

	remove_wait_queue(&sem->wait, wp);
	wp->proc->task.stat = READY; //通过修改队头，间接修改队头进程所指向的PCB

	do_free(wp);
}

//相当于sem->count += sleepers-1，然后返回sem->count
//将v指向的变量加上i，并测试结果是否为负。若为负，返回1，否则返回0
int add_negative(int i, int *v)
{	
	//此处if(i >= 0)是为了便于do_up之后进程再次进入do_down从而break 
	if(i >= 0) *v = i + *v; //added by mingxuan 2019-4-2
	if(*v < 0) return 1;
	//if((i + *v) < 0) return 1;
	else return 0;
}