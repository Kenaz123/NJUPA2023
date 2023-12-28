#include <common.h>
#include "syscall.h"
#include <proc.h>//newly added
// #define STRACE
/*int sys_write(int fd,void *buf,size_t count){
  if(fd == 1 || fd == 2){
    for(int i = 0; i < count; i++){
      putch(*((char *)buf + i));
    }
    return count;
  }
  return -1;
}*/
void naive_uload(PCB *pcb, const char *filename);

int sys_brk(void *addr){
  return 0;
}

int sys_execve(const char *fname) {
  naive_uload(NULL,fname);
  return -1;
}

void sys_exit(int status){
  //halt(status);
  sys_execve("/bin/nterm");
}

int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_write(int fd, const void *buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
int fs_close(int fd);


/*#define time_t uint64_t
#define suseconds_t uint64_t
struct timeval {
  time_t tv_sec;
  suseconds_t tv_usec;
};

struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};*/
#include <sys/time.h>
int sys_gettimeofday(struct timeval *tv,struct timezone *tz){
  uint64_t us = io_read(AM_TIMER_UPTIME).us;
  tv->tv_sec = us / 1000000;
  tv->tv_usec = us % 1000000;
  return 0;
}


#ifdef STRACE
#define PRINT_TRACE(name) Log(#name "(%d, %d, %d) = %d", c->GPR2, c->GPR3, c->GPR4, ret)
#else
#define PRINT_TRACE(name)
#endif

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  intptr_t ret;
  switch (a[0]) {
    case SYS_exit: printf("SYS_exit\n");sys_exit(c->GPR2);ret = 0;break;//SYS_exit
    case SYS_yield: printf("SYS_yield\n");yield();ret = 0;break;//SYS_yield
    case SYS_write: ret = fs_write(c->GPR2,(void *)c->GPR3,(size_t)c->GPR4);
      PRINT_TRACE(fs_write);
      break;
    case SYS_brk: ret = sys_brk((void *)c->GPR2);
      //Log("sys_brk(%d, %d, %d) = %d", c->GPR2, c->GPR3, c->GPR4, ret);
      PRINT_TRACE(sys_brk);
      break;
    case SYS_open: ret = fs_open((const char *)c->GPR2,c->GPR3,c->GPR4);
      //Log("fs_open(%s, %d, %d) = %d", (const char *)c->GPR2,c->GPR3,c->GPR4,ret);
      //PRINT_TRACE(fs_open);
      break;
    case SYS_read: ret = fs_read(c->GPR2,(void *)c->GPR3,(size_t)c->GPR4);
      //Log("fs_read(%d, %d, %d) = %d", c->GPR2,c->GPR3,c->GPR4,ret);
      PRINT_TRACE(fs_read);
      break;
    case SYS_close: ret = fs_close(c->GPR2);
      //Log("fs_close(%d) = %d",c->GPR2,ret);
      break;
    case SYS_lseek: ret = fs_lseek(c->GPR2, (size_t)c->GPR3, c->GPR4);
      PRINT_TRACE(fs_lseek);
      //Log("fs_lseek(%d,%d,%d) = %d",c->GPR2,(size_t)c->GPR3,c->GPR4,ret);
      break;
    case SYS_gettimeofday: ret = sys_gettimeofday((struct timeval *)c->GPR2, (struct timezone *)c->GPR3);
      PRINT_TRACE(sys_gettimeofday);
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
    case SYS_execve: ret = sys_execve((const char *)c->GPR2);
      PRINT_TRACE(sys_execve);
      while(1);
  }
  c->GPRx = ret;
}


