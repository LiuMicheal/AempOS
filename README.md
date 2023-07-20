# AempOS Introduction
---
 Over the past few decades, computer engineering has increasingly utilizd multiprocessing. While modern operating systems have the ability to take advantage of multiprocessor hardware, few educational operating systems exist to teach the principles of supporting such multiprocessor architectures. In order to provide students with a deeper understanding of the relationship between multiprocessor, parallelism, and kernel design, we introduce An educational multiprocessor OS, AempOS, which consists of seven experimental modules and four state-of-the-art features. Since 2015, AempOS has been used in the computer system related teaching of Northwestern Polytechnical University for 8 years. AempOS and its associated experimental design are open source, which can be accessed at: [AempOS's Labs](https://idealist226.github.io).

# AempOS Development Tools
---
AempOS is mainly developed based on C language and x86 assembly language, and the development tools used include:

* Assembler [nasm](https://www.nasm.us/)
* C language compiler gcc
* GNU Binary Toolset [Binutils](http://www.gnu.org/software/binutils/)
* Project build tool make
* Debugger gdb

Among them, Binutils is a set of tools for operating binary files, including the tool ar for creating static libraries, the tool strip for removing symbol tables from binary files to reduce file size, etc.

# Run AempOS
---
AempOS is currently booted from a floppy disk, and the boot process is as follows:
1. After the BIOS self-test is completed, the boot program (boot.bin) is loaded from the boot sector of the floppy disk to the memory, and the control is given to the boot program.
2. The boot program reads the loader (loader.bin) from the floppy disk to memory, and hands the controller to the loader.
3. When the loader is running, it will read the AempOS kernel (kernel.bin) from the floppy disk to the memory, then enter the protected mode from the real mode of the CPU, and hand over control to the kernel.
4. AempOS starts running.

Since AempOS is an operating system kernel for learners, it mainly runs in a virtual machine at present. The optional virtual machines are [Bochs](http://bochs.sourceforge.net/) and [Qemu](https:/ /www.qemu.org/).

**Run AempOS in Bochs**
1. Install Bochs. In Ubuntu system, you can directly execute the command `sudo apt-get install bochs` to install, or you can download the source code of Bochs first and then compile and install it. You can choose the desired Bochs version through the source code installation.
2. Enter the AempOS source directory, execute `tar zxvf misc/80m.img.tar.gz .`, and decompress the hard disk image from the hard disk image compression package.
3. Execute the `bochs` command in the current directory to start the Bochs virtual machine. Bochs will first read the configuration information from the bochsrc file, and then confirm the running prompt information given by Bochs to allow AempOS to run in Bochs.

**Run AempOS in Qemu**
1. According to Qemu, you can directly execute the command `sudo apt-get install qemu-system-x86` in the Ubuntu system, or you can download the source code of Qemu to compile and install.
2. Enter the AempOS source directory, execute `tar zxvf misc/80m.img.tar.gz .`, and decompress the hard disk image from the hard disk image compression package.
3. Execute the `./launch-qemu.sh` command in the current directory to start the Qemu virtual machine, and then AempOS will start running directly in Qemu. The Qemu virtual machine does not use the same configuration file as bochsrc, the configuration information is specified through command line options, and the script launch-qemu.sh contains the currently used configuration options.

# Debug AempOS
By using the built-in debugging function in Bochs or Qemu, AempOS can be debugged at the assembly language level, but because the assembly program is lengthy and difficult to read, this debugging method is not convenient to use. Fortunately, both Bochs and Qemu have built-in gdb support. By cooperating with the remote debugging function provided by gdb, AempOS can be debugged at the C source code level.

**Use Bochs+gdb to debug AempOS**
1. Compile and install Bochs from the source code, and enable the gdb support option when compiling. Then add gdb configuration information to the Bochs configuration file. The bochsrc-gdb file in the AempOS source directory already contains the required configuration options.
2. Execute `./launch-bochs-gdb.sh` in the AempOS source directory, the run shell script will run gdb in a new terminal window, and load the debug version of the kernel binary file.
3. Execute the command `target remote :2345` on the gdb command interface to establish a connection with Bochs.
4. Use gdb to debug AempOS like debugging a local program.

**Use Qemu+gdb to debug AempOS**
1. Add command-line options to enable gdb support when starting Qemu. The required configuration options have been added to the script file launch-qemu-gdb.sh in the AempOS source directory.
2. Execute `./launch-bochs-gdb.sh` in the AempOS source directory, the run shell script will run gdb in a new terminal window, and load the debug version of the kernel binary file.
3. Execute the command `target remote :1234` on the gdb command interface to establish a connection with Qemu.
4. Use gdb to debug AempOS like debugging a local program.

# Commonly used AempOS build options
```
# Compile the AempOS kernel and user program init, and write them into the floppy disk image a.img
make image
# Clear all .o object files
make clean
# Clear all .o object files and executables
make real clean
```

# References
* [Orange's](https://github.com/yyu/Oranges), due to a micro-operating system developed by Yuan, the development process of Orange's is described in the book "Implementation of an Operating System". AempOS is developed based on Orange's.
* [xv6](https://pdos.csail.mit.edu/6.828/2014/xv6.html), a micro-operating system developed by MIT for teaching, xv6 is rewritten from Unix V6 and used in MIT's operating system course 6.828: Operating System Engineering.
* [Minix](http://www.minix3.org/), a microkernel operating system originally developed by Professor Andrew S. Tanenbaum, Linus inherited many features from Minix when developing early Linux, Yu Yuan When developing Orange's, it also borrowed from Minix many times.