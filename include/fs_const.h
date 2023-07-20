/**
 * fs_const.h
 * This file contains consts and macros associated with filesystem.
 * The code is added by zcr, and the file is added by xw. 18/6/17
 */

/* TTY */
#define NR_CONSOLES	3	/* consoles */

/* max() & min() */
#define	max(a,b)	((a) > (b) ? (a) : (b))
#define	min(a,b)	((a) < (b) ? (a) : (b))

/* macros for messages */
#define	FD			u.m3.m3i1 
#define	PATHNAME	u.m3.m3p1 
#define	FLAGS		u.m3.m3i1 
#define	NAME_LEN	u.m3.m3i2 
#define	CNT			u.m3.m3i2
#define	REQUEST		u.m3.m3i2
#define	PROC_NR		u.m3.m3i3
#define	DEVICE		u.m3.m3i4
#define	POSITION	u.m3.m3l1
#define	BUF			u.m3.m3p2
#define	OFFSET		u.m3.m3i2 
#define	WHENCE		u.m3.m3i3 

/* #define	PID		u.m3.m3i2 */
/* #define	STATUS	u.m3.m3i1 */
#define	RETVAL		u.m3.m3i1
/* #define	STATUS	u.m3.m3i1 */


#define	DIOCTL_GET_GEO	1	//把请求的设备的起始扇区和扇区数目返回给调用者

/* 与硬盘扇区有关的宏 */
#define SECTOR_SIZE			512				  //每个扇区的大小，单位为Byte
#define SECTOR_BITS			(SECTOR_SIZE * 8) //每个扇区的大小，单位为bit
#define SECTOR_SIZE_SHIFT	9

/* 与主设备号有关的宏 */ //主设备号：每一类设备的名字
#define	NO_DEV			0
#define	DEV_FLOPPY		1
#define	DEV_CDROM		2
#define	DEV_HD			3
#define	DEV_CHAR_TTY	4
#define	DEV_SCSI		5
/* make device number from major and minor numbers */
#define	MAJOR_SHIFT		8
#define	MAKE_DEV(a,b)	((a << MAJOR_SHIFT) | b)
/* separate major and minor numbers from device number */
#define	MAJOR(x)		((x >> MAJOR_SHIFT) & 0xFF)	//由设备号得到主设备号
#define	MINOR(x)		(x & 0xFF)					//由设备号得到次设备号


#define	INVALID_INODE		0
#define	ROOT_INODE			1

/* 与硬盘分区数量有关的宏 */
#define	MAX_DRIVES			2	//只研究主IDE通道上连接2块硬盘的情况
#define	NR_PART_PER_DRIVE	4   //一块硬盘最多只能有4个分区(IBM认为一台PC最多会装4个操作系统)
#define	NR_SUB_PER_PART		16	//每个扩展分区最多只能有16个逻辑分区
#define	NR_SUB_PER_DRIVE	(NR_SUB_PER_PART * NR_PART_PER_DRIVE) //一块硬盘上最多有16*4=64个逻辑分区
#define	NR_PRIM_PER_DRIVE	(NR_PART_PER_DRIVE + 1) //整块硬盘(hd0)+4个主分区(hd[1~4]) = 5
#define	MAX_PRIM			(MAX_DRIVES * NR_PRIM_PER_DRIVE - 1) //主分区的最大次设备号
#define	MAX_SUBPARTITIONS	(NR_SUB_PER_DRIVE * MAX_DRIVES)	

/* 与次设备号有关的宏 */ //次设备号：每个设备(分区)的名字
#define	MINOR_hd1a		0x10
#define	MINOR_hd2a		(MINOR_hd1a+NR_SUB_PER_PART)
#define	MINOR_BOOT		MINOR_hd2a  				  //启动设备的次设备号  /// added by zcr
#define	ROOT_DEV		MAKE_DEV(DEV_HD, MINOR_BOOT)  //启动设备(根设备)的设备号(主设备号+次设备号)

/* 用在partition(int device, int style)中 */
#define	P_PRIMARY	0
#define	P_EXTENDED	1


#define ORANGES_PART	0x99	/* Orange'S partition */ //分区类型(SystemID)
#define NO_PART			0x00	/* unused entry */
#define EXT_PART		0x05	/* extended partition */


/* 以下是和文件系统相关 */
// #define	NR_FILES	64	//moved to proc.h. xw, 18/6/14
#define	NR_FILE_DESC	64	/* FIXME */
#define	NR_INODE	    64	/* FIXME */
#define	NR_SUPER_BLOCK	8

/* INODE::i_mode (octal, lower 32 bits reserved) */
#define I_TYPE_MASK     0170000
#define I_REGULAR       0100000
#define I_BLOCK_SPECIAL 0060000
#define I_DIRECTORY     0040000
#define I_CHAR_SPECIAL  0020000
#define I_NAMED_PIPE	0010000

#define	is_special(m)	((((m) & I_TYPE_MASK) == I_BLOCK_SPECIAL) ||	\
			 (((m) & I_TYPE_MASK) == I_CHAR_SPECIAL))

#define	NR_DEFAULT_FILE_SECTS	2048 /* 2048 * 512 = 1MB */ //默认每个文件占据2048个扇区

//deleted by xw, 18/12/27
//#define FSBUF_SIZE	0x100000	//added by xw, 18/6/17
