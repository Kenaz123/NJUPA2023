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
  for(int i = 0; i < len; i++) putch(*((char *)buf + i));
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  memset(temp_buf,0,TEMP_BUFSIZE);
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  if(ev.keycode == AM_KEY_NONE) return 0;
  const char *name = keyname[ev.keycode];
  int ret = ev.keydown ? sprintf(temp_buf,"kd %s\n",name) : sprintf(temp_buf,"ku %s\n",name);
  if(ret >= len){
    strncpy(buf,temp_buf,len-1);
    ret = len;
  } else {
    strncpy(buf,temp_buf,ret);
  }
  return ret;
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  memset(temp_buf,0,TEMP_BUFSIZE);
  AM_GPU_CONFIG_T ev = io_read(AM_GPU_CONFIG);
  int width = ev.width;
  int height = ev.height;
  int ret = sprintf(temp_buf, "WIDTH: %d\nHEIGHT: %d",width,height);
  if(ret >= len){
    strncpy(buf,temp_buf,len-1);
    ret = len;
  } else {
    strncpy(buf,temp_buf,ret);
  }
  return ret;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  return 0;
}

void init_device() {
  Log("Initializing devices...");
  //halt(-1);
  ioe_init();
}
