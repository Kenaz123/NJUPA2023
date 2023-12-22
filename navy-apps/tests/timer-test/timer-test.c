#include <stdio.h>
#include <sys/select.h>
#include <sys/types.h>

#define TEST_NDL

#ifndef TEST_NDL
#include <sys/time.h>
#include <assert.h>
void test1() {
  struct timeval start;
  struct timeval now;
  assert(gettimeofday(&start,NULL) == 0);
  time_t start_sec = start.tv_sec;
  suseconds_t start_usec = start.tv_usec;
  int times = 1;

  while(1){
    assert(gettimeofday(&now, NULL) == 0);
    time_t now_sec = now.tv_sec;
    suseconds_t now_usec = now.tv_usec;
    long int time_gap = (now_sec - start_sec) * 1000000 + (now_usec - start_usec);
    if(time_gap > 500000 * times){
      printf("Half a second to print %d time(s)\n",times);
      times++;
    }
  }
}
#else
#include <NDL.h>
void test2(){
  NDL_init(0);
  uint32_t init = NDL_GetTicks();
  int times = 1;

  while(1) {
    uint32_t now = NDL_GetTicks();
    uin32_t time_gap = now - init;
    if(time_gap > 500 * times) {
      printf("NDL Test: Half a second to print %d times\n",times);
      times++;
    }
  }
}
#endif

int main(){
#ifndef TEST_NDL
  test1();
#else
  test2();
#endif
}
