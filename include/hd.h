/*************************************************************************//**
 *****************************************************************************
 * @file   include/sys/hd.h
 * @brief  
 * @author Forrest Y. Yu
 * @date   2008
 *****************************************************************************
 *****************************************************************************/

#ifndef	_ORANGES_HD_H_
#define	_ORANGES_HD_H_

/**
 * @struct part_ent
 * @brief  Partition Entry struct.
 *
 * <b>Master Boot Record (MBR):</b>
 *   Located at offset 0x1BE in the 1st sector of a disk. MBR contains
 *   four 16-byte partition entries. Should end with 55h & AAh.
 *
 * <b>partitions in MBR:</b>
 *   A PC hard disk can contain either as many as four primary partitions,
 *   or 1-3 primaries and a single extended partition. Each of these
 *   partitions are described by a 16-byte entry in the Partition Table
 *   which is located in the Master Boot Record.
 *
 * <b>extented partition:</b>
 *   It is essentially a link list with many tricks. See
 *   http://en.wikipedia.org/wiki/Extended_boot_record for details.
 */
//硬盘分区表的表项
struct part_ent {
	u8 boot_ind; //状态

	u8 start_head;   //起始磁头号
	u8 start_sector; //起始扇区号(仅用了低6位，高2位为起始柱面号的第8,9位)
	u8 start_cyl;	 //起始柱面号的低8位
	
	u8 sys_id; //分区类型
	
	u8 end_head;   //结束磁头号
	u8 end_sector; //结束扇区号(仅用了低6位，高2位为结束柱面号的第8,9位)
	u8 end_cyl;	   //结束柱面号的低8位
	
	u32 start_sect; //起始扇区的LBA	
	u32 nr_sects;	//扇区数目

} PARTITION_ENTRY;


/********************************************/
/* I/O Ports used by hard disk controllers. */
/********************************************/
/* slave disk not supported yet, all master registers below */

/* Command Block Registers */
/*	MACRO		PORT			DESCRIPTION			INPUT/OUTPUT	*/
/*	-----		----			-----------			------------	*/
#define REG_DATA		0x1F0		  /*	Data				I/O		*/
#define REG_FEATURES	0x1F1		  /*	Features			O		*/

#define REG_ERROR		REG_FEATURES  /*	Error				I		*/
					/* 	The contents of this register are valid only when the error bit
						(ERR) in the Status Register is set, except at drive power-up or at the
						completion of the drive's internal diagnostics, when the register
						contains a status code.
						When the error bit (ERR) is set, Error Register bits are interpreted as such:
						|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						| BRK | UNC |     | IDNF|     | ABRT|TKONF| AMNF|
						+-----+-----+-----+-----+-----+-----+-----+-----+
						   |     |     |     |     |     |     |     |
						   |     |     |     |     |     |     |     `--- 0. Data address mark not found after correct ID field found
						   |     |     |     |     |     |     `--------- 1. Track 0 not found during execution of Recalibrate command
						   |     |     |     |     |     `--------------- 2. Command aborted due to drive status error or invalid command
						   |     |     |     |     `--------------------- 3. Not used
						   |     |     |     `--------------------------- 4. Requested sector's ID field not found
						   |     |     `--------------------------------- 5. Not used
						   |     `--------------------------------------- 6. Uncorrectable data error encountered
						   `--------------------------------------------- 7. Bad block mark detected in the requested sector's ID field
					*/
#define REG_NSECTOR		0x1F2		/*	Sector Count			I/O		*/

#define REG_LBA_LOW		0x1F3		/*	Sector Number / LBA Bits 0-7	I/O		*/
#define REG_LBA_MID		0x1F4		/*	Cylinder Low / LBA Bits 8-15	I/O		*/
#define REG_LBA_HIGH	0x1F5		/*	Cylinder High / LBA Bits 16-23	I/O		*/

#define REG_DEVICE		0x1F6		/*	Drive | Head | LBA bits 24-27	I/O		*/
					/*	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						|  1  |  L  |  1  | DRV | HS3 | HS2 | HS1 | HS0 |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						         |           |   \_____________________/
						         |           |              |
						         |           |              `------------ If L=0, Head Select.
						         |           |                                    These four bits select the head number.
						         |           |                                    HS0 is the least significant.
						         |           |                            If L=1, HS0 through HS3 contain bit 24-27 of the LBA.
						         |           `--------------------------- Drive. When DRV=0, drive 0 (master) is selected. 
						         |                                               When DRV=1, drive 1 (slave) is selected.
						         `--------------------------------------- LBA mode. This bit selects the mode of operation.
					 	                                                            When L=0, addressing is by 'CHS' mode.
					 	                                                            When L=1, addressing is by 'LBA' mode.
					*/
#define REG_STATUS		0x1F7		/*	Status				I		*/
					/* 	Any pending interrupt is cleared whenever this register is read.
						|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						| BSY | DRDY|DF/SE|  #  | DRQ |     |     | ERR |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						   |     |     |     |     |     |     |     |
						   |     |     |     |     |     |     |     `--- 0. Error.(an error occurred)
						   |     |     |     |     |     |     `--------- 1. Obsolete.
						   |     |     |     |     |     `--------------- 2. Obsolete.
						   |     |     |     |     `--------------------- 3. Data Request. (ready to transfer data)
						   |     |     |     `--------------------------- 4. Command dependent. (formerly DSC bit)
						   |     |     `--------------------------------- 5. Device Fault / Stream Error.
						   |     `--------------------------------------- 6. Drive Ready.
						   `--------------------------------------------- 7. Busy. If BSY=1, no other bits in the register are valid.
					*/
#define	STATUS_BSY	0x80
#define	STATUS_DRDY	0x40
#define	STATUS_DFSE	0x20
#define	STATUS_DSC	0x10
#define	STATUS_DRQ	0x08
#define	STATUS_CORR	0x04
#define	STATUS_IDX	0x02
#define	STATUS_ERR	0x01

#define REG_CMD		REG_STATUS	/*	Command				O		*/
					/*
						+--------+---------------------------------+-----------------+
						| Command| Command Description             | Parameters Used |
						| Code   |                                 | PC SC SN CY DH  |
						+--------+---------------------------------+-----------------+
						| ECh  @ | Identify Drive                  |             D   |
						| 91h    | Initialize Drive Parameters     |    V        V   |
						| 20h    | Read Sectors With Retry         |    V  V  V  V   |
						| E8h  @ | Write Buffer                    |             D   |
						+--------+---------------------------------+-----------------+
					
						KEY FOR SYMBOLS IN THE TABLE:
						===========================================-----=========================================================================
						PC    Register 1F1: Write Precompensation	@     These commands are optional and may not be supported by some drives.
						SC    Register 1F2: Sector Count		D     Only DRIVE parameter is valid, HEAD parameter is ignored.
						SN    Register 1F3: Sector Number		D+    Both drives execute this command regardless of the DRIVE parameter.
						CY    Register 1F4+1F5: Cylinder low + high	V     Indicates that the register contains a valid paramterer.
						DH    Register 1F6: Drive / Head
					*/

/* Control Block Registers */
/*	MACRO		PORT			DESCRIPTION			INPUT/OUTPUT	*/
/*	-----		----			-----------			------------	*/
#define REG_DEV_CTRL	0x3F6		/*	Device Control			O		*/
					/*	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						| HOB |  -  |  -  |  -  |  -  |SRST |-IEN |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						   |                             |     |
						   |                             |     `--------- Interrupt Enable.
						   |                             |                  - IEN=0, and the drive is selected,
						   |                             |                    drive interrupts to the host will be enabled.
						   |                             |                  - IEN=1, or the drive is not selected,
						   |                             |                    drive interrupts to the host will be disabled.
						   |                             `--------------- Software Reset.
						   |                                                - The drive is held reset when RST=1.
						   |                                                  Setting RST=0 re-enables the drive.
						   |                                                - The host must set RST=1 and wait for at least
						   |                                                  5 microsecondsbefore setting RST=0, to ensure
						   |                                                  that the drive recognizes the reset.
						   `--------------------------------------------- HOB (High Order Byte)
						                                                    - defined by 48-bit Address feature set.
					*/
#define REG_ALT_STATUS	REG_DEV_CTRL	/*	Alternate Status		I		*/
					/*	This register contains the same information as the Status Register.
						The only difference is that reading this register does not imply interrupt acknowledge or clear a pending interrupt.
					*/

#define REG_DRV_ADDR	0x3F7		/*	Drive Address			I		*/

//硬盘命令
struct hd_cmd
{
	u8	features;//写入REG_FEATURES(REG_FEATURES和REG_ERROR共用一个端口地址)
	u8	count;   //写入REG_NSECTOR //扇区数量
	u8	lba_low; //写入REG_LBA_LOW
	u8	lba_mid; //写入REG_LBA_MID	
	u8	lba_high;//写入REG_LBA_HIGH
	u8	device;	 //写入REG_DEVICE
	u8	command; //写入REG_CMD(REG_STATUS和REG_CMD共用一个端口地址)
};

//分区信息
struct part_info 
{
	u32	base;	//起始扇区LBA
	u32	size;	//扇区数目
};

//硬盘信息
/* main drive struct, one entry per drive */
struct hd_info
{
	int				 open_cnt;
	struct part_info primary[NR_PRIM_PER_DRIVE]; //记录所有主分区的起始扇区和扇区数目  NR_PRIM_PER_DRIVE=整块硬盘(hd0)+4个主分区(hd[1~4])=5
	struct part_info logical[NR_SUB_PER_DRIVE];	 //记录所有逻辑分区的起始扇区和扇区数目  NR_SUB_PER_DRIVE=一块硬盘上最多有16*4=64个逻辑分区
};


/***************/
/* DEFINITIONS */
/***************/
#define	HD_TIMEOUT				10000	/* in millisec */
#define	PARTITION_TABLE_OFFSET	0x1BE	//硬盘分区表是一个结构体数组，这个数组位于引导扇区的1BEh
#define ATA_IDENTIFY			0xEC
#define ATA_READ				0x20
#define ATA_WRITE				0x30

/* for DEVICE register. */
#define	MAKE_DEVICE_REG(lba,drv,lba_highest) (((lba) << 6) |		\
					      ((drv) << 4) |		\
					      (lba_highest & 0xF) | 0xA0)

// added by xw, 18/8/26
typedef struct rdwt_info
{
	MESSAGE *msg;
	void *kbuf;
	PROCESS *proc;
	struct rdwt_info *next;
} RWInfo;	//进程硬盘读写请求

typedef struct
{
	RWInfo *front;
	RWInfo *rear;
} HDQueue;

/* 以下是函数声明 */
PUBLIC void init_hd();
PUBLIC void hd_open(int device);
PUBLIC void hd_close(int device);

PUBLIC void hd_service();

PUBLIC void hd_rdwt(MESSAGE *p);
PUBLIC void hd_rdwt_sched(MESSAGE *p);	//异步硬盘读写函数
PUBLIC void hd_ioctl(MESSAGE *p);
//~xw

#endif /* _ORANGES_HD_H_ */
