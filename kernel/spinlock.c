/**********************************************************
*	spinlock.c       //added by mingxuan 2018-12-26
***********************************************************/

// Mutual exclusion spin locks.

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proc.h"
#include "global.h"
#include "spinlock.h"

//extern int use_console_lock;

static inline u32
cmpxchg(u32 oldval, u32 newval, volatile u32* lock_addr)
{
  u32 result;
  asm volatile("lock; cmpxchg %0, %2" :
               "+m" (*lock_addr), "=a" (result) :
               "r"(newval), "1"(oldval) :
               "cc");
  return result; 
}

void
initlock(struct spinlock *lock, char *name)
{
  lock->name = name;
  lock->locked = 0;
  lock->cpu = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// (Because contention is handled by spinning, must not
// go to sleep holding any locks.)
void
acquire(struct spinlock *lock)
{

  while(cmpxchg(0, 1, &lock->locked) == 1)
    ;
}

// Release the lock.
void
release(struct spinlock *lock)
{

  lock->locked = 0;
}



