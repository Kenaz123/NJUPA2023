//#include "amdev.h"
#include "am.h"
#include "amdev.h"
#include "klib-macros.h"
#include <common.h>
#include <stdio.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,
#define TEMP_BUFSIZE 64
static char temp_buf[TEMP_BUFSIZE];

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

size_t serial_write(const void *buf, size_t offset, size_t len) {
  //yield();
  for(int i = 0; i < len; i++) putch(*((char *)buf + i));
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  //yield();
  memset(temp_buf,'\0',TEMP_BUFSIZE);
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  if(ev.keycode == AM_KEY_NONE) return 0;
  const char *name = keyname[ev.keycode];
  int ret = ev.keydown ? sprintf(temp_buf,"kd %s\n",name) : sprintf(temp_buf,"ku %s\n",name);
  if(ret >= len){
    strncpy(buf,temp_buf,len-1);
    ((char*)buf)[len - 1]='\0';
    ret = len;
  } else {
    strncpy(buf,temp_buf,ret);
    ((char*)buf)[ret] = '\0';
  }
  return ret;
  //return snprintf((char*)buf, len, "%s %s\n",ev.keydown ? "kd" : "ku",keyname[ev.keycode]);
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  AM_GPU_CONFIG_T ev = io_read(AM_GPU_CONFIG);
  int width = ev.width;
  int height = ev.height;
  int ret = sprintf(buf, "WIDTH: %d\nHEIGHT: %d",width,height);
  return ret;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  //yield();
  AM_GPU_CONFIG_T ev = io_read(AM_GPU_CONFIG);
  int width = ev.width;
  offset /= 4;
  len /= 4;
  int y = offset / width;
  int x = offset % width;
  io_write(AM_GPU_FBDRAW,x,y,(void *)buf,len,1,true);
  return len;
}

void init_device() {
  Log("Initializing devices...");
  //halt(-1);
  ioe_init();
}
