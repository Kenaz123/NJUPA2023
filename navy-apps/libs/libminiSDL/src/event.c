#include "sdl-event.h"
#include <NDL.h>
#include <SDL.h>
//#include <SDL2/SDL_events.h>
//#include <stdlib.h>
//#include <SDL2/SDL_events.h>
#include <string.h>

#define keyname(k) #k,
int NDL_PollEvent(char *buf, int len); 
static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

static uint8_t keystate[sizeof(keyname)/sizeof(keyname[0])];

int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}

int SDL_PollEvent(SDL_Event *ev) {
  int buf_size = 32;
  //char *buf = (char *)malloc(buf_size * sizeof(char));
  char buf[buf_size];
  if(NDL_PollEvent(buf, buf_size) == 1) {
    if(strncmp(buf,"kd",2) == 0){
      ev->key.type = SDL_KEYDOWN;
    } else {
      ev->key.type = SDL_KEYUP;
    }
  
  //int flag = 0;
  for(int i = 0; i < sizeof(keyname) / sizeof(keyname[0]); i++){
    if(strncmp(buf + 3, keyname[i],strlen(buf) - 4) == 0 && strlen(keyname[i]) == strlen(buf) - 4) {
      //flag = 1;
      ev->key.keysym.sym = i;
      break;
    }
  }
  //assert(flag == 1);
  if(ev->key.type == SDL_KEYDOWN){
  keystate[ev->key.keysym.sym] = 1;
    } else {
      keystate[ev->key.keysym.sym] = 0;
    }
  //free(buf);
  return 1;

  } else {
    //ev->key.type = SDL_USEREVENT;
    //ev->key.keysym.sym = 0;
    //free(buf);
    return 0;
  }
}

int SDL_WaitEvent(SDL_Event *event) {
  while(SDL_PollEvent(event)==0);
  /*int buf_size = 32;
  char *buf = (char *)malloc(buf_size * sizeof(char));
  while(NDL_PollEvent(buf,buf_size) == 0);
  if(NDL_PollEvent(buf, buf_size) == 1) {
    if(strncmp(buf,"kd",2) == 0){
      event->key.type = SDL_KEYDOWN;
    } else {
      event->key.type = SDL_KEYUP;
    }
  
  //int flag = 0;
  for(int i = 0; i < sizeof(keyname) / sizeof(keyname[0]); i++){
    if(strncmp(buf + 3, keyname[i],strlen(buf) - 4) == 0 && strlen(keyname[i]) == strlen(buf) - 4) {
      //flag = 1;
      event->key.keysym.sym = i;
      break;
    }
  }
  //assert(flag == 1);
  
  free(buf);*/
  return 1;
  /*} else {
    event->key.type = SDL_USEREVENT;
    event->key.keysym.sym = 0;
    free(buf);
    return 1;
  }*/
  
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

//static unsigned char keystate[sizeof(keyname)/sizeof(keyname[0])];

uint8_t* SDL_GetKeyState(int *numkeys) {
  /*SDL_Event ev;
  memset(keystate, 0, sizeof(keystate));
  if(SDL_PollEvent(&ev) == 1 && ev.key.type == SDL_KEYDOWN){
    keystate[ev.key.keysym.sym] = 1;
  } else {
    //memset(keystate, 0, sizeof(keystate));
  }*/
  if(numkeys) *numkeys = 83;
  return keystate;
}
