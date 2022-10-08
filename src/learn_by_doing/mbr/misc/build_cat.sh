#!/bin/bash

AR=i686-elf-ar
GCC=i686-elf-gcc
LD=i686-elf-ld

if [ "$(uname -s)" == 'Linux' ]; then
    echo linux
    AR=ar
    GCC=gcc
    LD=ld
fi

nasm -f elf -o command/start.o command/start.S
${AR} rcs command/simple_crt.a command/start.o
${GCC} -c -Os -m32 -Wall -Wshadow -W -Wno-sign-conversion  -fno-stack-protector -fomit-frame-pointer -fno-builtin -fno-common  -ffreestanding  -Wno-unused-parameter -Wunused-variable -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-pic -I./device -I./kernel -I./lib -I./thread -I./userprog -I./fs -I./shell-m32 -o command/cat.o command/cat.c
${LD} -s -static -T ld_user.lds -n command/simple_crt.a command/cat.o kernel/syscall.o lib/stdio.o lib/string.o -o command/cat
dd if=command/cat of=bochs/disk0 bs=512 seek=100 conv=notrunc
