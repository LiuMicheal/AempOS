/******************************************************
*  ioapic.c         
*           added by mingxuan 2018-09-21
*******************************************************/

// The I/O APIC manages hardware interrupts for an SMP system.
// http://www.intel.com/design/chipsets/datashts/29056601.pdf
// See also picirq.c.

#include "type.h"
#include "const.h"

#define IOAPIC  0xFEC00000   // Default physical address of IO APIC

#define REG_ID     0x00  // Register index: ID
#define REG_VER    0x01  // Register index: version
#define REG_TABLE  0x10  // Redirection table base

// The redirection table starts at REG_TABLE and uses
// two registers to configure each interrupt.  
// The first (low) register in a pair contains configuration bits.
// The second (high) register contains a bitmask telling which
// CPUs can serve that interrupt.
#define INT_DISABLED   0x00010000  // Interrupt disabled
#define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
#define INT_ACTIVELOW  0x00002000  // Active low (vs high)
#define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)

#define T_IRQ0          32      // IRQ 0 corresponds to int T_IRQ //added by mingxuan 2019-3-4

volatile struct ioapic *ioapic;

extern int ismp; //added by mingxuan 2019-3-4
extern u8 ioapicid; //added by mingxuan 2019-3-4

// IO APIC MMIO structure: write reg, then read or write data.
struct ioapic {
  u32 reg;
  u32 pad[3];
  u32 data;
};

static u32
ioapicread(int reg)
{
  ioapic->reg = reg;
  return ioapic->data;
}

static void
ioapicwrite(int reg, u32 data)
{
  ioapic->reg = reg;
  ioapic->data = data;
}

void
ioapicinit(void)
{
  // added by mingxuan 2019-3-4
  // set 0xFEC00000 in page table,then hardware can recognize 0xFEC00000
  lin_mapping_ioapicPhy(0xFEC00000,//线性地址						
		       0xFEC00000,//物理地址				
		       PG_P  | PG_USU | PG_RWW,//页目录的属性位
		       PG_P  | PG_USU | PG_RWW);//页表的属性位
  
  int i, id, maxintr;

  if(!ismp)
    return;

  ioapic = (volatile struct ioapic*)IOAPIC;
  maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;
  id = ioapicread(REG_ID) >> 24;
  if(id != ioapicid)
    //cprintf("ioapicinit: id isn't equal to ioapicid; not a MP\n");
    disp_str("ioapicinit: id isn't equal to ioapicid; not a MP\n");

  // Mark all interrupts edge-triggered, active high, disabled,
  // and not routed to any CPUs.
  for(i = 0; i <= maxintr; i++){
    ioapicwrite(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
    ioapicwrite(REG_TABLE+2*i+1, 0);
  }
}

void
ioapicenable(int irq, int cpunum)
{
  if(!ismp)
    return;

  // Mark interrupt edge-triggered, active high,
  // enabled, and routed to the given cpunum,
  // which happens to be that cpu's APIC ID.
  ioapicwrite(REG_TABLE+2*irq, T_IRQ0 + irq);
  ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);
}
