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
CFLAGS          = -c -Os -std=c99 -m32 -Wall -Wshadow -W -Wconversion -Wno-sign-conversion  -fno-stack-protector -fomit-frame-pointer -fno-builtin -fno-common  -ffreestanding  -Wno-unused-parameter -Wunused-variable -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables
LDFLAGS         = -s -static -T hello.lds -n -Map HelloOS.map
OJCYFLAGS       = -S -O binary


bin: mbr.bin loader.bin kernel.bin
	@ls *.bin

kernel.bin: kernel.o vgastr.o
	$(LD) -s -static -m elf_i386 -Ttext 0xc0001500 -e main -o $@ kernel.o vgastr.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@	

%.bin: %.asm
	$(ASM) -o $@ $<

clean:
	$(RM) -f *.o *.bin
