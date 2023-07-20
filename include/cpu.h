/**********************************************************
*	cpu.h       //added by mingxuan 2018-12-26
***********************************************************/

//就绪队列的每个节点  //added by mingxuan 2019-4-3
struct ready_proc
{
	PROCESS *proc;
  struct ready_proc *prev; //added by mingxuan 2019-4-12
	struct ready_proc *next;
};

//就绪队列  //added by mingxuan 2019-4-3
struct ready_queue
{
	struct ready_proc *front; //就绪队列的头指针
	struct ready_proc *rear;  //就绪队列的尾指针
};

struct GDT_PTR
{
	u16 gdt_limit;
	u32 gdt_base;
};
//结构体中嵌套结构体时，一定要单独定义内层结构体

// Per-CPU state
struct cpu {
  u8 id;                    // Local APIC ID; index into cpulist[] below

  struct GDT_PTR *gdt_ptr;		//added by mingxuan 2018-12-22 //结构体中嵌套结构体时，一定要单独定义内层结构体
  DESCRIPTOR gdt[GDT_SIZE]; 	//added by mingxuan 2018-12-22

  TSS tss;                     //added by mingxuan 2019-1-21
  
  //volatile u32 started;       // Has the CPU started?
  int started;                  // modified by mingxuan 2019-4-1

  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?

  //PROCESS *ready_proc;
  //int ready_proc[N_READY_PROC];  //added by mingxuan 2019-2-28 //deleted by mingxuan 2019-4-3
  struct ready_queue ready;    //modified by mingxuan 2019-4-3
  int nr_ready_proc;			    //当前就绪队列中进程的总数	//added by mingxuan 2019-4-3

  // Cpu-local storage variables; see below
  struct cpu *cpu;
  PROCESS *proc;           // The currently-running process.
};

//#define NCPU         2  // maximum number of CPUs //deleted by mingxuan 2019-1-22

extern struct cpu cpulist[NCPU];
extern int cpunumber;	                    //added by mingxuan 2018-11-14
extern struct GDT_PTR gdt_ptr_list[NCPU]; //结构体中嵌套结构体时，一定要单独定义内层结构体 //added by mingxuan 2019-4-1

extern struct cpu *cpu asm("%fs:0");	// added by mingxuan 2018-11-14
extern PROCESS *proc asm("%fs:4");	// added by mingxuan 2018-11-14

//enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNINGA, ZOMBIE };
/*
struct mp {             // floating pointer
  u8 signature[4];           // "_MP_"
  void *physaddr;               // phys addr of MP config table
  u8 length;                 // 1
  u8 specrev;                // [14]
  u8 checksum;               // all bytes must add up to 0
  u8 type;                   // MP system config type
  u8 imcrp;
  u8 reserved[3];
};

struct mpconf {         // configuration table header
  u8 signature[4];           // "PCMP"
  u16 length;                // total table length
  u8 version;                // [14]
  u8 checksum;               // all bytes must add up to 0
  u8 product[20];            // product id
  u32 *oemtable;               // OEM table pointer
  u16 oemlength;             // OEM table length
  u16 entry;                 // entry count
  u32 *lapicaddr;              // address of local APIC
  u16 xlength;               // extended table length
  u8 xchecksum;              // extended table checksum
  u8 reserved;
};

struct mpproc {         // processor table entry
  u8 type;                   // entry type (0)
  u8 apicid;                 // local APIC id
  u8 version;                // local APIC verison
  u8 flags;                  // CPU flags
    #define MPBOOT 0x02           // This proc is the bootstrap processor.
  u8 signature[4];           // CPU signature
  u32 feature;                 // feature flags from CPUID instruction
  u8 reserved[8];
};

struct mpioapic {       // I/O APIC table entry
  u8 type;                   // entry type (2)
  u8 apicno;                 // I/O APIC id
  u8 version;                // I/O APIC version
  u8 flags;                  // I/O APIC flags
  u32 *addr;                  // I/O APIC address
};

// Table entry types
#define MPPROC    0x00  // One per processor
#define MPBUS     0x01  // One per bus
#define MPIOAPIC  0x02  // One per I/O APIC
#define MPIOINTR  0x03  // One per bus interrupt source
#define MPLINTR   0x04  // One per system interrupt source
*/

// lapic.c
//extern volatile u32* lapic;

//added by mingxuan 2019-5-7
#define CPU0_ID 0
#define CPU1_ID 1
#define CPU2_ID 2
#define CPU3_ID 3
