build: print.o mycrt.o
	@ld -e mystart -static mycrt.o print.o
no_crt0_no_main_exe: mymain.o
	@ld -e mymain -static -o mymain mymain.o
mymain.o: mymain.c
	@gcc mymain.c -c -fno-stack-protector
print.o: print.c
	@gcc -c print.c -fno-stack-protector
mycrt.o: mycrt.asm
	@nasm -felf64 mycrt.asm
