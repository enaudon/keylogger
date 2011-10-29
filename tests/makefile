syscall: syscall.o
	ld -o syscall syscall.o
syscall.o: syscall.asm
	nasm -f elf -g -F stabs syscall.asm
