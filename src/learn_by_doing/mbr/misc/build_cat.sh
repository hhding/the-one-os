i686-elf-ar rcs command/simple_crt.a command/start.o
i686-elf-gcc -c -Os -m32 -Wall -Wshadow -W -Wno-sign-conversion  -fno-stack-protector -fomit-frame-pointer -fno-builtin -fno-common  -ffreestanding  -Wno-unused-parameter -Wunused-variable -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-pic -I./device -I./kernel -I./lib -I./thread -I./userprog -I./fs -I./shell-m32 -o command/cat.o command/cat.c
i686-elf-ld -s -static -T ld.lds -n command/simple_crt.a command/cat.o kernel/syscall.o lib/stdio.o lib/string.o -o command/cat
