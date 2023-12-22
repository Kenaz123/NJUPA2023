#include <stdio.h>
#include <assert.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

int main(){
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
