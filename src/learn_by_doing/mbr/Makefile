MAKEFLAGS = -sR
MKDIR = mkdir
RMDIR = rmdir
CP = cp
CD = cd
DD = dd
RM = rm

ASM             = nasm
CC              = gcc
LD              = ld

ASMBFLAGS       = -f elf -w-orphan-labels
CFLAGS          = -c -Os -m32 -Wall -Wshadow -W -Wno-sign-conversion  -fno-stack-protector -fomit-frame-pointer -fno-builtin -fno-common  -ffreestanding  -Wno-unused-parameter -Wunused-variable -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-pic
LDFLAGS         = -static -T ld.lds -n -Map hello.map
#LDFLAGS         = -s -static -T ld.lds -n -Map hello.map

objfiles = main.o printk.o interrupt.o interrupt_helper.o debug.o string.o bitmap.o memory.o thread.o list.o switch.o timer.o sync.o keyboard.o

bin: mbr.bin loader.bin kernel.bin
	@dd if=mbr.bin of=/mnt/d/mbr bs=512 seek=0 conv=notrunc
	@dd if=loader.bin of=/mnt/d/mbr bs=512 seek=1 conv=notrunc
	@dd if=kernel.bin of=/mnt/d/mbr bs=512 seek=9 conv=notrunc

kernel.bin: $(objfiles)
	$(LD) $(LDFLAGS) -o $@ $(objfiles)

interrupt_helper.o: interrupt_helper.asm
	$(ASM) -f elf -o $@ $<

switch.o: switch.asm
	$(ASM) -f elf -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@	

%.bin: %.asm
	$(ASM) -o $@ $<

clean:
	$(RM) -f *.o *.bin *.map
