MAKEFLAGS = -sR
MKDIR = mkdir
RMDIR = rmdir
CP = cp
CD = cd
DD = dd
RM = rm

ASM             = nasm
CC              = i686-elf-gcc
LD              = i686-elf-ld

detected_os := $(shell sh -c 'uname -s')

ifeq ($(detected_os),Linux)
    CC              = gcc
    LD              = ld
endif


ASMBFLAGS       = -f elf -w-orphan-labels
CFLAGS          = -c -Os -m32 -Wall -Wshadow -W -Wno-sign-conversion  -fno-stack-protector -fomit-frame-pointer -fno-builtin -fno-common  -ffreestanding  -Wno-unused-parameter -Wunused-variable -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-pic -I./device -I./kernel -I./lib -I./thread -I./userprog -I./fs -I./shell
LDFLAGS         = -static -T ld.lds -n -Map hello.map
#LDFLAGS         = -s -static -T ld.lds -n -Map hello.map

objfiles = kernel/main.o lib/printk.o kernel/interrupt.o kernel/interrupt_helper.o kernel/debug.o lib/string.o lib/bitmap.o kernel/memory.o thread/thread.o lib/list.o kernel/switch.o device/timer.o kernel/sync.o device/keyboard.o kernel/tss.o userprog/process.o kernel/syscall.o lib/stdio.o device/ide.o fs/fs.o fs/inode.o fs/file.o fs/dir.o userprog/fork.o shell/pipe.o shell/shell.o device/ioqueue.o shell/buildin_cmd.o

bin: boot/mbr.bin boot/loader.bin kernel/kernel.bin
	@dd if=boot/mbr.bin of=bochs/disk0 bs=512 seek=0 conv=notrunc
	@dd if=boot/loader.bin of=bochs/disk0 bs=512 seek=1 conv=notrunc
	@dd if=kernel/kernel.bin of=bochs/disk0 bs=512 seek=9 conv=notrunc
	@nm kernel/kernel.bin | cut -d' ' -f1,3 > bochs/sym.dat

run: bin
	cd bochs && bochs -f bochsrc.bxrc

kernel/kernel.bin: $(objfiles)
	$(LD) $(LDFLAGS) -o $@ $(objfiles)

kernel/interrupt_helper.o: kernel/interrupt_helper.asm
	$(ASM) -f elf -o $@ $<

kernel/switch.o: kernel/switch.asm
	$(ASM) -f elf -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@	

boot/mbr.bin: boot/mbr.asm
	$(ASM) -o $@ $<

%.bin: %.asm
	$(ASM) -o $@ $<

clean:
	$(RM) -f */*.o */*.bin *.map
