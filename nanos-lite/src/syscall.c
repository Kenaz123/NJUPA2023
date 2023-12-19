#include <common.h>
#include "syscall.h"

int sys_write(int fd,void *buf,size_t count){
  //if(fd == 1 || fd == 2){
    for(int i = 0; i < count; i++){
      putch(*((char *)buf + i));
    }
    return count;
  //}
  //return -1;
}

int sys_brk(void *addr){
  return 0;
}

void sys_exit(int status){
  halt(status);
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  //intptr_t ret;
  switch (a[0]) {
    case SYS_exit: printf("SYS_exit\n");sys_exit(c->GPR2);c->GPRx = 0;break;//SYS_exit
    case SYS_yield: printf("SYS_yield\n");yield();c->GPRx = 0;break;//SYS_yield
    case SYS_write: c->GPRx = sys_write(c->GPR2,(void *)c->GPR3,(size_t)c->GPR4);
       Log("sys_write(%d, %d, %d) = %d", c->GPR2, c->GPR3, c->GPR4, c->GPRx);break;
    case SYS_brk: c->GPRx = sys_brk((void *)c->GPR2);
      Log("sys_brk(%d, %d, %d) = %d", c->GPR2, c->GPR3, c->GPR4, c->GPRx);break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
  //c->GPRx = ret;
}


