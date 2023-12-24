#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;
static int canvas_w = 0, canvas_h = 0;
static int canvas_x = 0, canvas_y = 0;

uint32_t NDL_GetTicks() {
  struct timeval tv;
  assert(gettimeofday(&tv, NULL) == 0);
  return (tv.tv_sec*1000 + tv.tv_usec / 1000);
}

//static int event_fd = 0;

int NDL_PollEvent(char *buf, int len) {
  memset(buf, 0, len);
  int fd = open("/dev/events", 0, 0);
  int ret = read(fd, buf, len);
  //assert(close(fd) == 0);
  return ret == 0 ? 0 : 1;
}

void NDL_OpenCanvas(int *w, int *h) {
  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }
  int buf_size = 64;
  char *buf = (char *)malloc(buf_size * sizeof(char));
  int fd = open("/proc/dispinfo", 0, 0);
  int ret = read(fd, buf, buf_size);
  //assert(ret < buf_size);
  //assert(close(fd) == 0);
  
  //int i = 0;
  assert(strncmp(buf,"WIDTH",5) == 0);
  int width = 0, height = 0;
  sscanf(buf,"WIDTH: %d\nHEIGHT: %d",&width,&height);
  //printf("%s\n", buf);
  free(buf);
  screen_w = width;
  screen_h = height;
  if(*w == 0 && *h == 0){
    *w = screen_w;
    *h = screen_h;
  }
  canvas_w = *w;
  canvas_h = *h;
  canvas_x = (screen_w - canvas_w)/2;
  canvas_y = (screen_h - canvas_h)/2;
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  int fd = open("/dev/fb", 0, 0);
  // printf("canvas_w: %d\ncanvas_h: %d\n",canvas_w,canvas_h);
  for(int i = 0; i < h && y + i < canvas_h; i++){
  //printf("%d %d %d %d\n",canvas_y,canvas_x,screen_w , x);
  // printf("%d\n",((y + canvas_y + i) * screen_w + canvas_x + x) * 4);
    lseek(fd,((y + canvas_y + i) * screen_w + canvas_x + x) * 4,SEEK_SET);
    write(fd,pixels + i * w,((w + x < canvas_w) ? w : (canvas_w - x))*4);
  }
  //assert(close(fd) == 0);
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  //event_fd = open("dev/events", 0, 0);
  return 0;
}

void NDL_Quit() {
}
