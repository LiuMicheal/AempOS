/******************************************************
*  lapic.c     local APIC manages internal interrupts    
*           added by mingxuan 2018-09-21
*******************************************************/

// The local APIC manages internal (non-I/O) interrupts.
// See Chapter 8 & Appendix C of Intel processor manual volume 3.

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "string.h"
#include "global.h"
//#include "cpu.h"  //deleted by mingxuan 2019-1-22

// Local APIC registers, divided by 4 for use as u32[] indices.
#define ID      (0x0020/4)   // ID
#define VER     (0x0030/4)   // Version
#define TPR     (0x0080/4)   // Task Priority
#define EOI     (0x00B0/4)   // EOI
#define SVR     (0x00F0/4)   // Spurious Interrupt Vector
  #define ENABLE     0x00000100   // Unit Enable
#define ESR     (0x0280/4)   // Error Status
#define ICRLO   (0x0300/4)   // Interrupt Command
  #define INIT       0x00000500   // INIT/RESET
  #define STARTUP    0x00000600   // Startup IPI
  #define DELIVS     0x00001000   // Delivery status
  #define ASSERT     0x00004000   // Assert interrupt (vs deassert)
  #define DEASSERT   0x00000000
  #define LEVEL      0x00008000   // Level triggered
  #define BCAST      0x00080000   // Send to all APICs, including self.
  #define BUSY       0x00001000
  #define FIXED      0x00000000
#define ICRHI   (0x0310/4)   // Interrupt Command [63:32]

//3个和局部时钟相关的宏，mingxuan
#define TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
  #define X1         0x0000000B   // divide counts by 1
  #define X64        0x00000009   // divide counts by 64  //added by mingxuan 2019-2-26
  #define X128       0x0000000A   // divide counts by 128 //added by mingxuan 2019-2-26
  #define PERIODIC   0x00020000   // Periodic
  #define ONE_SHOT   0x00000000   // One-shot //added by mingxuan 2019-2-26

#define PCINT   (0x0340/4)   // Performance Counter LVT
#define LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
#define LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
#define ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
  #define MASKED     0x00010000   // Interrupt masked

//3个和局部时钟相关的寄存器，mingxuan
#define TICR    (0x0380/4)   // Timer Initial Count
#define TCCR    (0x0390/4)   // Timer Current Count
#define TDCR    (0x03E0/4)   // Timer Divide Configuration

#define T_IRQ0          32      // IRQ 0 corresponds to u32 T_IRQ

#define IRQ_TIMER        0
#define IRQ_KBD          1
#define IRQ_COM1         4
#define IRQ_IDE         14
#define IRQ_ERROR       19
#define IRQ_SPURIOUS    31

volatile u32 *lapic;  // Initialized in mp.c

static void
lapicw(u32 index, u32 value)
{
  lapic[index] = value;
  lapic[ID];  // wait for write to finish, by reading
}
//PAGEBREAK!

void
lapicinit(void)
{
  // added by mingxuan 2018-10-17
  // set 0xFEE00000 in page table,then hardware can recognize 0xFEE00000
  lin_mapping_lapicPhy(0xFEE00000,//线性地址						
		       0xFEE00000,//物理地址				
		       PG_P  | PG_USU | PG_RWW,//页目录的属性位
		       PG_P  | PG_USU | PG_RWW);//页表的属性位

  // The following is cpoied from xv6
  if(!lapic) 
    return;

  // Enable local APIC; set spurious interrupt vector.
  lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));
  //lapicw(SVR, ENABLE);  //added by mingxuan 2019-2-27

  // The timer repeatedly counts down at bus frequency
  // from lapic[TICR] and then issues an interrupt.  
  // If xv6 cared more about precise timekeeping,            //如果需要更精准的计时,
  // TICR would be calibrated using an external time source. //可以使用外部时间源校准TICR
  lapicw(TDCR, X1);
  //lapicw(TDCR, X64);  //added by mingxuan 2019-2-26
  //lapicw(TDCR, X128); //added by mingxuan 2019-2-26
  lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
  //lapicw(TIMER, ONE_SHOT | (T_IRQ0 + IRQ_TIMER)); //added by mingxuan 2019-2-26
  //lapicw(TICR, 10000000); 
  lapicw(TICR, 100000000);  //设置TICR初值为0x1000,为了加快局部时钟 //modified by mingxuan 2019-1-26
  //TDCR,TIMER,TICR都是32位的寄存器

  /*
  //测试时钟中断
  //added by mingxuan 2019-2-26
  char *p_IRR_32_40 = 0xFEE00220; //IRR的第32~40位(IRR总共有255位,0xFEE00200~0xFEE00270),局部时钟中断对应第32位
	char *p_ISR_32_40 = 0xFEE00120;	//ISR的第32~40位(ISR总共有255位,0xFEE00100~0xFEE00170),局部时钟中断对应第32位
  while(1)
  {
      //if(0 == lapic[TCCR] && 0x20820 == lapic[TIMER])
      if(0 == lapic[TCCR])
      {
          disp_str("TCCR:");
          disp_int(lapic[TCCR]);
          disp_str(" ");
          
          disp_str("LVT0:");
          disp_int(lapic[TIMER]);
          disp_str(" ");

          disp_str("IRR:");
			    disp_int(*p_IRR_32_40);
			    disp_str(" ");

			    disp_str("ISR:");
			    disp_int(*p_ISR_32_40);
			    disp_str(" ");

          //disp_str("\n");
      }
  }
  

  //added by mingxuan 2019-1-26
  disp_str(" ");
  disp_int(lapic[TCCR]);
  disp_str("-");
  disp_int(lapic[ID]>>24);
  disp_str(" ");
  */


  // Disable logical interrupt lines.
  lapicw(LINT0, MASKED);
  lapicw(LINT1, MASKED);

  // Disable performance counter overflow interrupts
  // on machines that provide that interrupt entry.
  if(((lapic[VER]>>16) & 0xFF) >= 4)
    lapicw(PCINT, MASKED);

  // Map error interrupt to IRQ_ERROR.
  lapicw(ERROR, T_IRQ0 + IRQ_ERROR);

  // Clear error status register (requires back-to-back writes).
  lapicw(ESR, 0);
  lapicw(ESR, 0);

  // Ack any outstanding interrupts.
  lapicw(EOI, 0);

  // Send an Init Level De-Assert to synchronise arbitration ID's.
  lapicw(ICRHI, 0);
  lapicw(ICRLO, BCAST | INIT | LEVEL);
  while(lapic[ICRLO] & DELIVS)
    ;

  // Enable interrupts on the APIC (but not on the processor).
  lapicw(TPR, 0); //设置TPR为0，意味着处理器将处理所有中断

  //added by mingxuan 2019-1-26
  /*
  disp_int(lapic[TCCR]);
  disp_str("-");
  disp_int(lapic[ID]>>24);
  disp_str(" ");
  */

}

//added by mingxuan 2018-11-15
int
cpunum(void)
{
  if(lapic)
    return lapic[ID]>>24;
  return 0;
}

// Acknowledge interrupt.
void
lapiceoi(void)
{
  if(lapic)
    lapicw(EOI, 0);
}

// Spin for a given number of microseconds.
// On real hardware would want to tune this dynamically.
void
microdelay(u32 us)
{
}

#define CMOS_PORT    0x70
#define CMOS_RETURN  0x71

// Start additional processor running entry code at addr.
// See Appendix B of MultiProcessor Specification.
void
lapicstartap(u8 apicid, u32 addr)
{
  u32 i;
  u16 *wrv;

  // "The BSP must initialize CMOS shutdown code to 0AH
  // and the warm reset vector (DWORD based at 40:67) to point at
  // the AP startup code prior to the [universal startup algorithm]."
  out_byte(CMOS_PORT, 0xF);  // offset 0xF is shutdown code
  out_byte(CMOS_PORT+1, 0x0A);
  wrv = (u16*)((0x40<<4 | 0x67));  // Warm reset vector
  wrv[0] = 0;
  wrv[1] = addr >> 4;

  // "Universal startup algorithm."
  // Send INIT (level-triggered) interrupt to reset other CPU.
  lapicw(ICRHI, apicid<<24);
  lapicw(ICRLO, INIT | LEVEL | ASSERT);
  microdelay(200);
  lapicw(ICRLO, INIT | LEVEL);
  microdelay(100);    // should be 10ms, but too slow in Bochs!

  // Send startup IPI (twice!) to enter code.
  // Regular hardware is supposed to only accept a STARTUP
  // when it is in the halted state due to an INIT.  So the second
  // should be ignored, but it is part of the official Intel algorithm.
  // Bochs complains about the second one.  Too bad for Bochs.
  for(i = 0; i < 2; i++){
    lapicw(ICRHI, apicid<<24);
    lapicw(ICRLO, STARTUP | (addr>>12));
    microdelay(200);
  }
}

//added by mingxuan 2019-1-26
/*
void
print_TCCR(void)
{
 disp_int(lapic[TCCR]);
 disp_str(" ");
}
*/