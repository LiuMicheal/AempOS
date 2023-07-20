/**********************************************************
*	rwlock.h       //added by mingxuan 2019-4-8
***********************************************************/

/*
* 这个锁标志与自旋锁不一样，自旋锁的lock标志只能取0和1两种值。
* 读写自旋锁的lock分两部分：
*     0-23位：表示并发读的数量。数据以补码的形式存放。
*     24位：未锁标志。如果没有读或写时设置该，否则清0
* 注意：如果自旋锁为空（设置了未锁标志并且无读者），则lock字段为0x01000000
*     如果写者获得了锁，则lock为0x00000000（未锁标志清0，表示已经锁，但是无读者）
*     如果一个或者多个进程获得了读锁，那么lock的值为0x00ffffff,0x00fffffe等（未锁标志清0，后面跟读者数量的补码）
*/
struct rwlock 
{
    volatile unsigned int lock;
};

#define RW_LOCK_DEFAULT 0x01000000 //读写锁的初值为0x01000000

void init_rwlock(struct rwlock *rw);

static inline void read_lock(struct rwlock *rw)
{
	asm volatile("lock ; subl $1,(%0)\n\t"  //锁的初始值是0x01000000，每个做读操作的进程对rw减1，并判断
		        "jns 1f\n"				    //如果rw的值大于或等于0,表示读者成功获得了锁	
		        "call read_lock_failed\n\t" //如果rw的值小于0，则读者获取读锁失败(说明此时有写者或读者太多了),进入read_lock_failed持续等待另一进程对写锁的释放
		        "1:"
		        :
		        :"r"(rw->lock)
		        :"memory"
		        );
}

static inline void write_lock(struct rwlock *rw)
{
	asm volatile("lock ; subl %1,(%0)\n\t" 	  //锁的初始值是0x0100000，每个做写操作的进程对rw减0x0100000，并判断
		        "jz 2f\n"				 	  //如果rw的值为0,表示该写者成功获得了锁			
		        "call write_lock_failed\n\t"  //如果rw的值为非零, 则获取写锁失败(说明此时有多个读者),进入write_lock_failed持续等待所有进程对读锁的释放
		        "2:"
		        :
		        :"r"(rw->lock), "i"(RW_LOCK_DEFAULT)
		        :"memory"
		        );
}

static inline void read_unlock(struct rwlock *rw)
{
	asm volatile("lock ; incl %0" //将rw变量加1
		        :"+m"(rw->lock)
		        :
		        :"memory"
		        );
}

static inline void write_unlock(struct rwlock *rw)
{
	asm volatile("lock ; addl %1,%0" //将rw变量加0x01000000
		        :"+m"(rw->lock)
		        :"i"(RW_LOCK_DEFAULT)
		        :"memory"
		        );
}

//rwlock暂无用处，若使用，还需调试 //added by mingxuan 2019-4-8