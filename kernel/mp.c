/******************************************************
*  mp.c  added by mingxuan 2018-11-27
*******************************************************/
#include "type.h"
#include "const.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "string.h"
#include "global.h"
#include "cpu.h"   //deleted by mingxuan 2019-1-22
#include "mp.h"    //added by mingxuan 2019-1-22

int cpunumber = 0;	               //CPU数量
struct cpu cpulist[NCPU];          //mp系统中所有CPU
struct GDT_PTR gdt_ptr_list[NCPU]; //结构体中嵌套结构体时，一定要单独定义内层结构体 //added by mingxuan 2019-4-1
int ismp;	                         //标志位，表示是否是MP系统
u8 ioapicid;

//extern volatile u32* lapic; //added by mingxuan 2019-1-22

static u8 sum(u8 *addr, int len)
{
  int i, sum;
  
  sum = 0;
  for(i=0; i<len; i++)
    sum += addr[i];
  return sum;
}

//内存比较
int memcmp(const void *v1, const void *v2, u32 n)
{
  const u8 *s1, *s2;
  
  s1 = v1;
  s2 = v2;
  while(n-- > 0){
    if(*s1 != *s2)
      return *s1 - *s2;
    s1++, s2++;
  }

  return 0;
}

// 在从地址a开始,长度len中寻找MP浮点结构,找到后返回其地址指针
static struct mp* mpsearch1(u32 a, int len)
{
  u8 *e, *p, *addr;
  addr = (a);
  e = addr+len;
  for(p = addr; p < e; p += sizeof(struct mp))
    if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
      return (struct mp*)p;
  return 0;
}

// 在以下3个地方寻找MP浮点结构
// 1）EBDA(如果存在的话)的第一个KB
// 2）(EBDA不存在的情况下)系统基本内存的最后一个KB
// 3）BIOS ROM的0xF0000到0xFFFFF
static struct mp* mpsearch(void)
{
  u8 *bda;
  u32 p;
  struct mp *mp;
  bda = (u8 *) (0x400);
  if((p = ((bda[0x0F]<<8)| bda[0x0E]) << 4)){
    if((mp = mpsearch1(p, 1024)))
      return mp;
  } else {
    p = ((bda[0x14]<<8)|bda[0x13])*1024;
    if((mp = mpsearch1(p-1024, 1024)))
      return mp;
  }
  return mpsearch1(0xF0000, 0x10000);
}

//检查MP配置表头并返回其物理地址。
static struct mpconf* mpconfig(struct mp **pmp)
{
  struct mpconf *conf;
  struct mp *mp;

  if((mp = mpsearch()) == 0 || mp->physaddr == 0)
    return 0;
  conf = (struct mpconf*) ((u32) mp->physaddr);
  if(memcmp(conf, "PCMP", 4) != 0)
    return 0;
  if(conf->version != 1 && conf->version != 4)
    return 0;
  if(sum((u8*)conf, conf->length) != 0)
    return 0;
  *pmp = mp;
  return conf;
}

//寻找CPU并存入cpulist中
PUBLIC void mpinit()
{
  u8 *p, *e;
  struct mp *mp;
  struct mpconf *conf;
  struct mpioapic *ioapic;

  if((conf = mpconfig(&mp)) == 0)    //得到MP配置表头的指针并检查其是否不为0
    return;
  ismp = 1;

  lapic = (u32*)conf->lapicaddr;
  //disp_int(lapic);			//add by mingxuan 2018.10.16

  //MP配置表的扩展部分由五种入口组成,此处为遍历扩展部分
  for(p=(u8*)(conf+1), e=(u8*)conf+conf->length; p<e; ){  
    switch(*p){
    case MPPROC:                           //入口类型为CPU
      cpulist[cpunumber].id = cpunumber;   //将此CPU存入cpulist中并给定其id
      cpunumber++;                         //CPU数量+1
      p += sizeof(struct mpproc);    
      continue; 
    case MPIOAPIC:                         //入口类型为I/OAPIC
      ioapic = (struct mpioapic*)p;
      ioapicid = ioapic->apicno;
      p += sizeof(struct mpioapic);
      continue;
    case MPBUS:                            //入口类型为总线
    case MPIOINTR:                         //入口类型为I/O中断分配
    case MPLINTR:                          //入口类型为逻辑中断分配
      p += 8;
      continue;
    default:
      ismp = 0;
    }
  }
  if(!ismp){
    // 非多核系统
    cpunumber = 1;
    lapic = 0;
    ioapicid = 0;
    return;
  }
  disp_str("[cpunumber:");
  disp_int(cpunumber);         //屏幕显示CPU个数
  disp_str("]");
}

