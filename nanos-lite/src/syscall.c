#include <common.h>
#include "syscall.h"
void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;

  switch (a[0]) {
    case 0: c->GPRx=0;halt(c->GPRx);//SYS_exit
    case 1: yield();
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
