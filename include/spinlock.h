/**********************************************************
*	spinlock.h       //added by mingxuan 2018-12-26
***********************************************************/

// Mutual exclusion lock.
//#define uint unsigned
struct spinlock {
  u32 locked;   // Is the lock held?
  
  // For debugging:
  char *name;     // Name of lock.
  struct cpu *cpu; // The cpu holding the lock. //modified by mingxuan 2019-1-16
  u32 pcs[10];   // The call stack (an array of program counters)
                  // that locked the lock.
};

void initlock(struct spinlock *lock, char *name);
// Acquire the lock.
// Loops (spins) until the lock is acquired.
// (Because contention is handled by spinning, must not
// go to sleep holding any locks.)
void acquire(struct spinlock *lock);
// Release the lock.
void release(struct spinlock *lock);





