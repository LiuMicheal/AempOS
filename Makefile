#########################
# Makefile for Orange'S #
#########################

# Entry point of Orange'S
# It must have the same value with 'KernelEntryPointPhyAddr' in load.inc!
#############edit by visual 2016.5.10####
ENTRYPOINT	= 0xC0030400


# Offset of entry point in kernel file
# It depends on ENTRYPOINT
ENTRYOFFSET	=   0x400

# Programs, flags, etc.
ASM		= nasm
DASM	= ndisasm
CC		= gcc
LD		= ld
AR		= ar

ASMBFLAGS	= -I boot/include/
ASMKFLAGS	= -I include/ -f elf
CFLAGS		= -I include/ -c -fno-builtin -fno-stack-protector -m32	-Wall -Wextra -g # added by mingxuan 2018-12-11
LDFLAGS		= -s -Ttext $(ENTRYPOINT) -m elf_i386 # added by mingxuan 2018-12-11
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)
ARFLAGS		= rcs

# CFLAGS		= -I include/ -c -fno-builtin -fno-stack-protector
# CFLAGS		= -I include/ -m32 -c -fno-builtin -fno-stack-protector
#modified by xw
# CFLAGS		= -I include/ -m32 -c -fno-builtin -fno-stack-protector -Wall -Wextra -g # deleted by mingxuan 2018-12-11
# CFLAGS_app	= -I include/ -m32 -c -fno-builtin -fno-stack-protector -Wall -Wextra -g # deleted by mingxuan 2018-12-11

# LDFLAGS		= -s -Ttext $(ENTRYPOINT)
# LDFLAGS		= -m elf_i386 -s -Ttext $(ENTRYPOINT)
#generate map file. added by xw
LDFLAGS_kernel	= -m elf_i386 -s -Ttext $(ENTRYPOINT) -Map misc/kernel.map
LDFLAGS_init	= -m elf_i386 -s -Map init/init.map
LDFLAGS_init1	= -m elf_i386 -s -Map init/init1.map	#added by mingxuan 2019-3-7
LDFLAGS_init2	= -m elf_i386 -s -Map init/init2.map	#added by mingxuan 2019-3-14
LDFLAGS_init3	= -m elf_i386 -s -Map init/init3.map	#added by mingxuan 2019-3-14
#discard -s, so keep symbol information that gdb can use. added by xw
LDFLAGS_kernel_gdb	= -m elf_i386 -Ttext $(ENTRYPOINT)
LDFLAGS_init_gdb	= -m elf_i386
LDFLAGS_init1_gdb	= -m elf_i386	#added by mingxuan 2019-3-7	
LDFLAGS_init2_gdb	= -m elf_i386	#added by mingxuan 2019-3-14	
LDFLAGS_init3_gdb	= -m elf_i386	#added by mingxuan 2019-3-14

# This Program
ORANGESBOOT	= boot/boot.bin boot/loader.bin
ORANGESKERNEL	= kernel.bin
ORANGESINIT	= init/init.bin
ORANGESINIT1= init/init1.bin	#added by mingxuan 2019-3-7
ORANGESINIT2= init/init2.bin	#added by mingxuan 2019-3-14
ORANGESINIT3= init/init3.bin	#added by mingxuan 2019-3-14
OBJS		= kernel/kernel.o kernel/syscall.o kernel/start.o kernel/main.o kernel/clock.o\
			kernel/i8259.o kernel/global.o kernel/protect.o kernel/proc.o\
			lib/kliba.o lib/klib.o lib/string.o kernel/syscallc.o kernel/memman.o kernel/pagetbl.o	\
			kernel/elf.o kernel/file.o kernel/exec.o kernel/fork.o kernel/pthread.o \
			kernel/ktest.o kernel/testfunc.o kernel/fs.o kernel/hd.o \
			kernel/spinlock.o kernel/mp.o kernel/lapic.o kernel/ioapic.o kernel/keyboard.o kernel/tty.o \
			kernel/semaphore.o kernel/rwlock.o kernel/ipc.o kernel/argget.o kernel/msgqueue.o kernel/box.o # added by mingxuan 2019-5-14
OBJSINIT	= init/init.o init/initstart.o lib/ulib.a 
OBJSINIT1	= init/init1.o init/initstart1.o lib/ulib.a	#added by mingxuan 2019-3-7
OBJSINIT2	= init/init2.o init/initstart2.o lib/ulib.a	#added by mingxuan 2019-3-14
OBJSINIT3	= init/init3.o init/initstart3.o lib/ulib.a	#added by mingxuan 2019-3-14
OBJSULIB 	= lib/string.o kernel/syscall.o
DASMOUTPUT	= kernel.bin.asm
#added by xw
#GDBBIN = kernel.gdb.bin init/init.gdb.bin	#deleted by mingxuan 2019-3-7
GDBBIN = kernel.gdb.bin init/init1.gdb.bin	#modified by mingxuan 2019-3-7

# All Phony Targets
.PHONY : everything final image clean realclean disasm all buildimg tags

# Default starting position
nop :
	@echo "why not \`make image' huh? :)"

#everything : $(ORANGESBOOT) $(ORANGESKERNEL) $(ORANGESINIT) $(GDBBIN)	#deleted by mingxuan 2019-3-7
#everything : $(ORANGESBOOT) $(ORANGESKERNEL) $(ORANGESINIT) $(ORANGESINIT1) $(GDBBIN)	#modified by mingxuan 2019-3-7
everything : $(ORANGESBOOT) $(ORANGESKERNEL) $(ORANGESINIT) $(ORANGESINIT1) $(ORANGESINIT2) $(ORANGESINIT3) $(GDBBIN) #modified by mingxuan 2019-3-14

all : realclean everything

# image : realclean everything clean buildimg
# image : everything buildimg tags #deleted by mingxuan 2019-1-5
image : realclean everything clean buildimg tags 

clean :
	#rm -f $(OBJS) $(OBJSINIT)	#deleted by mingxuan 2019-3-7
	#rm -f $(OBJS) $(OBJSINIT) $(OBJSINIT1)	#modified by mingxuan 2019-3-7 
	rm -f $(OBJS) $(OBJSINIT) $(OBJSINIT1) $(OBJSINIT2) $(OBJSINIT3)	#modified by mingxuan 2019-3-14

realclean :
	#rm -f $(OBJS) $(OBJSINIT) $(ORANGESBOOT) $(ORANGESKERNEL) $(ORANGESINIT) $(GDBBIN) kernel/entryother.bin #deleted by mingxuan 2019-3-7
	#rm -f $(OBJS) $(OBJSINIT) $(OBJSINIT1) $(ORANGESBOOT) $(ORANGESKERNEL) $(ORANGESINIT) $(ORANGESINIT1) $(GDBBIN) #modified by mingxuan 2019-3-7
	rm -f $(OBJS) $(OBJSINIT) $(OBJSINIT1) $(OBJSINIT2) $(OBJSINIT3) $(ORANGESBOOT) $(ORANGESKERNEL) $(ORANGESINIT) $(ORANGESINIT1) $(ORANGESINIT2) $(ORANGESINIT3) $(GDBBIN) #modified by mingxuan 2019-3-14

disasm :
	$(DASM) $(DASMFLAGS) $(ORANGESKERNEL) > $(DASMOUTPUT)

# We assume that "a.img" exists in current folder
buildimg :
	dd if=boot/boot.bin of=a.img bs=512 count=1 conv=notrunc
	tar -zxvf 80m.img.tar.gz				#added by mingxuan 2019-3-22
	sudo mount -o loop a.img /mnt/floppy/
	sudo cp -fv boot/loader.bin /mnt/floppy/
	sudo cp -fv kernel.bin 		/mnt/floppy
	sudo cp -fv init/init.bin 	/mnt/floppy
	sudo cp -fv init/init1.bin 	/mnt/floppy	#added by mingxuan 2019-3-7
	sudo cp -fv init/init2.bin 	/mnt/floppy	#added by mingxuan 2019-3-14
	sudo cp -fv init/init3.bin 	/mnt/floppy	#added by mingxuan 2019-3-14
#	sudo cp -fv command/echo.bin /mnt/floppy
	sudo umount /mnt/floppy

# generate tags file. added by xw, 19/1/2
tags :
	ctags -R	

archive :
	git archive --prefix=

boot/boot.bin : boot/boot.asm boot/include/load.inc boot/include/fat12hdr.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

boot/loader.bin : boot/loader.asm boot/include/load.inc boot/include/fat12hdr.inc boot/include/pm.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

#$(ORANGESKERNEL) : $(OBJS)
#	$(LD) $(LDFLAGS) -o $(ORANGESKERNEL) $(OBJS)
#modified by xw, 18/6/10
#	$(LD) $(LDFLAGS) -Map kernel.map -o $(ORANGESKERNEL) $(OBJS)
#modified by xw, 18/6/12
#	$(LD) $(LDFLAGS_kernel) -o $(ORANGESKERNEL) $(OBJS)
#modified by mingxuan 2019-1-5
$(ORANGESKERNEL) : $(OBJS) kernel/entryother.bin
	$(LD) -Ttext 0xC0030400 -m elf_i386 -Map kernel.map -o $(ORANGESKERNEL) $(OBJS) -b binary kernel/entryother.bin
	
#$(ORANGESINIT) : $(OBJSINIT)
#	$(LD) -s -o $(ORANGESINIT) $(OBJSINIT)
#	$(LD) -m elf_i386 -s -o $(ORANGESINIT) $(OBJSINIT)
#modified by xw, 18/6/11
#	$(LD) -m elf_i386 -s -Map init/init.map -o $(ORANGESINIT) $(OBJSINIT)
#modified by xw, 18/6/12
#	$(LD) $(LDFLAGS_init) -o $(ORANGESINIT) $(OBJSINIT)
#modified by mingxuan 2019-1-5
$(ORANGESINIT): $(OBJSINIT)
	$(LD) -s -m elf_i386 -Map init/init.map -o $(ORANGESINIT) $(OBJSINIT)
#added by mingxuan 2019-3-7
$(ORANGESINIT1): $(OBJSINIT1)
	$(LD) -s -m elf_i386 -Map init/init1.map -o $(ORANGESINIT1) $(OBJSINIT1)
#added by mingxuan 2019-3-14
$(ORANGESINIT2): $(OBJSINIT2)
	$(LD) -s -m elf_i386 -Map init/init2.map -o $(ORANGESINIT2) $(OBJSINIT2)
#added by mingxuan 2019-3-14
$(ORANGESINIT3): $(OBJSINIT3)
	$(LD) -s -m elf_i386 -Map init/init3.map -o $(ORANGESINIT3) $(OBJSINIT3)

#added by xw
kernel.gdb.bin : $(OBJS)
	$(LD) $(LDFLAGS_kernel_gdb) -o $@ $(OBJS) -b binary kernel/entryother.bin # modified by mingxuan 2019-1-5

init/init.gdb.bin : $(OBJSINIT)
	$(LD) $(LDFLAGS_init_gdb) -o $@ $(OBJSINIT)
#added by mingxuan 2019-3-7
init/init1.gdb.bin : $(OBJSINIT1)
	$(LD) $(LDFLAGS_init1_gdb) -o $@ $(OBJSINIT1)
#added by mingxuan 2019-3-14
init/init2.gdb.bin : $(OBJSINIT2)
	$(LD) $(LDFLAGS_init2_gdb) -o $@ $(OBJSINIT2)
#added by mingxuan 2019-3-14
init/init3.gdb.bin : $(OBJSINIT3)
	$(LD) $(LDFLAGS_init3_gdb) -o $@ $(OBJSINIT3)
	
kernel/kernel.o : kernel/kernel.asm include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/syscall.o : kernel/syscall.asm include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/start.o: kernel/start.c include/type.h include/const.h include/protect.h include/string.h include/proc.h include/proto.h \
			include/global.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/main.o: kernel/main.c include/type.h include/const.h include/protect.h include/string.h include/proc.h include/proto.h \
			include/global.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/clock.o: kernel/clock.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/i8259.o: kernel/i8259.c include/type.h include/const.h include/protect.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/global.o: kernel/global.c include/type.h include/const.h include/protect.h include/proc.h \
			include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/protect.o: kernel/protect.c include/type.h include/const.h include/protect.h include/proc.h include/proto.h \
			include/global.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/proc.o: kernel/proc.c
	$(CC) $(CFLAGS) -o $@ $<

lib/klib.o: lib/klib.c include/type.h include/const.h include/protect.h include/string.h include/proc.h include/proto.h \
			include/global.h
	$(CC) $(CFLAGS) -o $@ $<

lib/kliba.o : lib/kliba.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/string.o : lib/string.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/syscallc.o: kernel/syscallc.c include/type.h include/const.h include/protect.h include/proto.h \
			include/string.h include/proc.h include/global.h
	$(CC) $(CFLAGS) -o $@ $<	

kernel/memman.o: kernel/memman.c /usr/include/stdc-predef.h include/memman.h include/type.h include/const.h include/protect.h \
 			include/proto.h include/string.h include/proc.h include/global.h
	$(CC) $(CFLAGS) -o $@ $<
	
kernel/pagetbl.o: kernel/pagetbl.c include/type.h include/const.h include/protect.h include/proto.h include/string.h \
			include/proc.h include/global.h
	$(CC) $(CFLAGS) -o $@ $<	

lib/ulib.a:  $(OBJSULIB)
	$(AR) $(ARFLAGS) -o $@  $(OBJSULIB)
	
init/init.o: init/init.c include/stdio.h
	# $(CC) $(CFLAGS_app) -o $@ $<
	$(CC) $(CFLAGS) -o $@ $< #modified by mingxuan 2018-12-11
	
#added by mingxuan 2019-3-7
init/init1.o: init/init1.c include/stdio.h
	# $(CC) $(CFLAGS_app) -o $@ $<
	$(CC) $(CFLAGS) -o $@ $< #modified by mingxuan 2018-12-11

#added by mingxuan 2019-3-14
init/init2.o: init/init2.c include/stdio.h
	# $(CC) $(CFLAGS_app) -o $@ $<
	$(CC) $(CFLAGS) -o $@ $< #modified by mingxuan 2018-12-11

#added by mingxuan 2019-3-14
init/init3.o: init/init3.c include/stdio.h
	# $(CC) $(CFLAGS_app) -o $@ $<
	$(CC) $(CFLAGS) -o $@ $< #modified by mingxuan 2018-12-11

init/initstart.o: init/initstart.asm 
	$(ASM) $(ASMKFLAGS) -o $@ $<

#added by mingxuan 2019-3-7
init/initstart1.o: init/initstart1.asm 
	$(ASM) $(ASMKFLAGS) -o $@ $<

#added by mingxuan 2019-3-14
init/initstart2.o: init/initstart2.asm 
	$(ASM) $(ASMKFLAGS) -o $@ $<

#added by mingxuan 2019-3-14
init/initstart3.o: init/initstart3.asm 
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/elf.o: kernel/elf.c /usr/include/stdc-predef.h include/type.h include/const.h include/protect.h \
			include/proto.h include/string.h include/proc.h include/global.h include/elf.h
	$(CC) $(CFLAGS) -o $@ $<
	
kernel/file.o: kernel/file.c /usr/include/stdc-predef.h include/type.h include/const.h include/protect.h \
			include/proto.h include/string.h include/proc.h include/global.h
	$(CC) $(CFLAGS) -o $@ $<
	
kernel/exec.o: kernel/exec.c /usr/include/stdc-predef.h include/type.h include/const.h include/protect.h \
			include/proto.h include/string.h include/proc.h include/global.h include/elf.h
	$(CC) $(CFLAGS) -o $@ $<
	
kernel/fork.o: kernel/fork.c /usr/include/stdc-predef.h include/type.h include/const.h include/protect.h \
			include/proto.h include/string.h include/proc.h include/global.h
	$(CC) $(CFLAGS) -o $@ $<
	
kernel/pthread.o: kernel/pthread.c /usr/include/stdc-predef.h include/type.h include/const.h include/protect.h \
			include/proto.h include/string.h include/proc.h include/global.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/ktest.o: kernel/ktest.c include/type.h include/const.h include/protect.h include/string.h include/proc.h \
			include/proto.h include/global.h include/cpu.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/testfunc.o: kernel/testfunc.c include/type.h include/const.h include/protect.h include/string.h include/proc.h \
			include/proto.h include/global.h
	$(CC) $(CFLAGS) -o $@ $<
	
# fs.o and hd.o; added by xw, 18/5/25
kernel/fs.o: kernel/fs.c include/type.h include/const.h include/protect.h include/string.h include/proc.h \
			include/proto.h include/global.h include/fs_const.h include/fs.h include/hd.h include/fs_misc.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/hd.o: kernel/hd.c include/type.h include/const.h include/protect.h include/string.h include/proc.h \
			include/proto.h include/global.h include/fs_const.h include/fs.h include/hd.h include/fs_misc.h
	$(CC) $(CFLAGS) -o $@ $<

# spinlock.o	added by mingxuan 2019-1-5
kernel/spinlock.o: kernel/spinlock.c include/spinlock.h
	$(CC) -masm=intel $(CFLAGS) -o $@ $<

# entryother.bin	added by mingxuan 2018-12-20
kernel/entryother.bin : kernel/entryother.asm boot/include/pm.inc boot/include/load.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

# mp.o	added by mingxuan 2018-12-13
kernel/mp.o: kernel/mp.c include/type.h include/const.h include/protect.h \
			include/proto.h include/string.h include/proc.h include/global.h include/cpu.h
	$(CC) $(CFLAGS) -o $@ $<

# lapic.o	added by mingxuan 2018-12-13
kernel/lapic.o: kernel/lapic.c include/type.h include/const.h include/protect.h \
			include/proto.h include/string.h include/proc.h include/global.h include/cpu.h
	$(CC) $(CFLAGS) -o $@ $<

# ioapic.o	added by mingxuan 2019-3-4
kernel/ioapic.o: kernel/ioapic.c include/type.h include/const.h include/protect.h \
			include/proto.h include/string.h include/proc.h include/global.h include/cpu.h
	$(CC) $(CFLAGS) -o $@ $<

# keyboard.o added by mingxuan 2019-3-8
kernel/keyboard.o: kernel/keyboard.c include/type.h include/const.h include/protect.h \
			include/proto.h include/string.h include/proc.h include/global.h include/cpu.h include/keymap.h include/keyboard.h
	$(CC) $(CFLAGS) -o $@ $<

# tty.o	added by mingxuan 2019-3-8
kernel/tty.o: kernel/tty.c
	$(CC) $(CFLAGS) -o $@ $<

# semaphore.o added by mingxuan 2019-3-28
kernel/semaphore.o: kernel/semaphore.c include/semaphore.h
	$(CC) $(CFLAGS) -o $@ $<

# semaphore.o added by mingxuan 2019-4-8
kernel/rwlock.o: kernel/rwlock.c include/rwlock.h
	$(CC) $(CFLAGS) -o $@ $<

# ipc.o added by mingxuan 2019-5-13
kernel/ipc.o: kernel/ipc.c include/type.h include/const.h include/protect.h \
			include/proto.h include/string.h include/proc.h include/global.h 
	$(CC) $(CFLAGS) -o $@ $<

# argget.o added by mingxuan 2019-5-13
kernel/argget.o: kernel/argget.c include/type.h include/const.h include/protect.h \
			include/proto.h include/string.h include/proc.h include/global.h
	$(CC) $(CFLAGS) -o $@ $<

# msgqueue.o added by mingxuan 2019-5-13
kernel/msgqueue.o: kernel/msgqueue.c include/type.h include/const.h include/protect.h \
			include/proto.h include/string.h include/proc.h include/global.h include/msgqueue.h 
	$(CC) $(CFLAGS) -o $@ $<

# box.o added by mingxuan 2019-5-14
kernel/box.o: kernel/box.c include/type.h include/const.h include/protect.h \
			include/proto.h include/string.h include/proc.h include/global.h include/box.h 
	$(CC) $(CFLAGS) -o $@ $<