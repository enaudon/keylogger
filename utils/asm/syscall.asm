SECTION .text

GLOBAL _start   ;declare entry point for linker

_start:
  nop           ;keeps gdb happy

;test make system call
  mov eax, 20   ;syscall index
  int 80h       ;make kernel call

;exit the program
  mov eax, 1    ;index   1 = sys_exit
  mov ebx, 0    ;ret val 0 = normal
  int 80h       ;make kernel call

