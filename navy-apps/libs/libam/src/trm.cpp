#include <am.h>

Area heap;
#define nemu_trap(code) asm volatile("mv a0, %0; .word 0x0000006b" : :"r"(code))

void putch(char ch) {
  putchar(ch);
}

void halt(int code) {
  nemu_trap(code);
  while(1);
}
