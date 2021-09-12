build: print.o test.o
	@ld -e mystart --dynamic-linker=/lib64/ld-linux-x86-64.so.2 test.o print.o -lc
no_crt0_no_main_exe: print.o
	@ld -e print --dynamic-linker=/lib64/ld-linux-x86-64.so.2 print.o -lc
print.o: print.c
	@gcc -c print.c
test.o: test.asm
	@nasm -felf64 test.asm
