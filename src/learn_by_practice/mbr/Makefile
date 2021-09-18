mbr: mbr.asm
	@nasm mbr.asm
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
CFLAGS          = -c -Os -std=c99 -Wall -Wshadow -W -Wconversion -Wno-sign-conversion  -fno-stack-protector -fomit-frame-pointer -fno-builtin -fno-common  -ffreestanding  -Wno-unused-parameter -Wunused-variable
LDFLAGS         = -s -static -T hello.lds -n -Map HelloOS.map
OJCYFLAGS       = -S -O binary

all: asm_crt mymain

asm_crt: print.o mycrt.o
	$(LD) -e mystart -static -o $@ print.o mycrt.o

mymain: mymain.o
	$(LD) -e mymain -static -o $@ mymain.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@	

%.o: %.asm
	$(ASM) -felf64 -o $@ $<

clean:
	$(RM) -f *.o a.out asm_crt mymain
