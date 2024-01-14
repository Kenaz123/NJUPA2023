#include <nterm.h>
#include <stdarg.h>
#include <unistd.h>
#include <SDL.h>
#include <string.h>//newly added

char handle_key(SDL_Event *ev);

static void sh_printf(const char *format, ...) {
  static char buf[256] = {};
  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buf, 256, format, ap);
  va_end(ap);
  term->write(buf, len);
}

static void sh_banner() {
  sh_printf("Built-in Shell in NTerm (NJU Terminal)\n\n");
}

static void sh_prompt() {
  sh_printf("sh> ");
}

//static char fname[64];
static void sh_handle_cmd(const char *cmd) {
  if (cmd == NULL) return;
  if (strncmp(cmd, "echo", 4) == 0) {
    if(strlen(cmd) == 5) sh_printf("\n");
    else sh_printf("%s",cmd + 5);
  } else {
    /*if(strlen(cmd) > 64) {
      sh_printf("command too long\n");
      return;*/
    int argc = 0;
    char *cmd_n = (char *)malloc(sizeof(char) * strlen(cmd));
    memset(cmd_n, 0, strlen(cmd));
    strncpy(cmd_n, cmd, strlen(cmd) - 1);
    
    char **argv = (char **)malloc(sizeof(char *) * 16);//argc_max = 16
    char *cur = strtok(cmd_n," ");
    assert(cur != NULL);

    char *fname = (char *)malloc(sizeof(char) * (strlen(cur) + 1));
    memset(fname, 0, strlen(cur) + 1);
    strcpy(fname, cur);

    while(cur){
      argv[argc] = cur;
      cur = strtok(NULL," ");
      argc++;
      if(argc == 16){
        sh_printf("too many arguments\n");
        free(argv);
        free(fname);
        free(cmd_n);
        return;
      }
    }
    argv[argc] = NULL;
    execve(fname, argv, NULL);
    free(argv);
    free(fname);
    free(cmd_n);
    }
    /*memset(fname, 0, 64);
    strncpy(fname, cmd, strlen(cmd) - 1);//remove line break
    execvp(fname, NULL);*/

  
}

void builtin_sh_run() {
  sh_banner();
  sh_prompt();
  assert(setenv("PATH","/bin", 0) == 0);
  while (1) {
    SDL_Event ev;
    if (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_KEYUP || ev.type == SDL_KEYDOWN) {
        const char *res = term->keypress(handle_key(&ev));
        if (res) {
          sh_handle_cmd(res);
          sh_prompt();
        }
      }
    }
    refresh_terminal();
  }
}
