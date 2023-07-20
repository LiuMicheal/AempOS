/******************************************************
*  
*  Routines to let C code use special x86 instructions.    
*  added by mingxuan 2018-12-21
*        
*******************************************************/

static inline void
lcr3(u32 val)
{
	asm volatile("movl %0, %%cr3" 			 
		     :		//输出
		     :"r"(val)	//输入
		    ); 	
}

static inline u32
xchg(volatile u32 *addr, u32 newval)
{
	u32 result;
  
        // The + in "+m" denotes a read-modify-write operand.
	asm volatile("lock; xchgl %0, %1" 
		     :"+m" (*addr), "=a" (result) 
                     :"1" (newval) 
                     :"cc");
        return result;
}

static inline void
sgdt(u8 cpuid)
{
	volatile u16 pd[3];
	u16 *result = pd;

	asm volatile("sgdt (%0)" 			 
		     :"=r"(result)  //输出		
		     :		    //输入
		    ); 		

	struct cpu *c;
	c = cpulist + cpuid;
	
	c->gdt_ptr->gdt_base = pd[1] + (pd[2]<<16);
	c->gdt_ptr->gdt_limit = pd[0];
} 

static inline void
sidt(u8 cpuid)
{
	volatile u16 pd[3];
	u16 *result = pd;

	asm volatile("sidt (%0)" 			 
		     :"=r"(result)  //输出		
		     :		    //输入
		    ); 		

	struct cpu *c;
	c = cpulist + cpuid;
	
	c->gdt_ptr->gdt_base = pd[1];
	c->gdt_ptr->gdt_limit = pd[0];
} 


static inline void
lgdt(DESCRIPTOR *p, int size)
{
 	volatile u16 pd[3];

	pd[0] = size;
	pd[1] = (u32)p;
	pd[2] = (u32)p >> 16;

	asm volatile("lgdt (%0)" 
		     : 
                     :"r"(pd)
		    );
}

static inline void
lidt(GATE *p, int size)
{
  	volatile u16 pd[3];

  	pd[0] = size;
  	pd[1] = (u32)p;
  	pd[2] = (u32)p >> 16;

  	asm volatile("lidt (%0)" 
		     : 
		     :"r" (pd)
		    );
}

static inline void
ltr(u16 sel)
{
  asm volatile("ltr %0" : : "r" (sel));
}

static inline void
loadfs(u16 v)
{
  asm volatile("movw %0, %%fs" : : "r" (v));
}

//added by mingxuan 2019-1-26
static inline void
cli(void)
{
  asm volatile("cli");
}

//added by mingxuan 2019-1-26
static inline void
sti(void)
{
  asm volatile("sti");
}