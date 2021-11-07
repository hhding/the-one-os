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
OBJCOPY = objcopy

ASMBFLAGS       = -f elf -w-orphan-labels
#CFLAGS          = -c -Os -std=c99 -m32 -Wall -Wshadow -W -Wconversion -Wno-sign-conversion  -fno-stack-protector -fomit-frame-pointer -fno-builtin -fno-common  -ffreestanding  -Wno-unused-parameter -Wunused-variable
#CFLAGS          = -c -Os -std=c99 -m32 -Wall -Wshadow -W -Wconversion -Wno-sign-conversion  -fno-stack-protector -fomit-frame-pointer -fno-builtin -fno-common  -ffreestanding  -Wno-unused-parameter -Wunused-variable -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables
CFLAGS          = -c -Os -m32 -Wall -Wshadow -W -Wno-sign-conversion  -fno-stack-protector -fomit-frame-pointer -fno-builtin -fno-common  -ffreestanding  -Wno-unused-parameter -Wunused-variable -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables
LDFLAGS         = -s -static -T ld.lds -n -Map hello.map
OJCYFLAGS       = -S -O binary


bin: mbr.bin loader.bin kernel.bin
	@dd if=mbr.bin of=/mnt/d/mbr bs=512 seek=0 conv=notrunc
	@dd if=loader.bin of=/mnt/d/mbr bs=512 seek=1 conv=notrunc
	@dd if=kernel.bin of=/mnt/d/mbr bs=512 seek=9 conv=notrunc

kernel.bin: kernel.o vgastr.o
	$(LD) $(LDFLAGS) -o $@ kernel.o vgastr.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@	

%.bin: %.asm
	$(ASM) -o $@ $<

clean:
	$(RM) -f *.o *.bin *.map
