#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[], char *envp[]);
extern char **environ;
void call_main(uintptr_t *args) {
  uintptr_t *base = args;
  int argc = *base;
  base += 1;
  char **argv = (char **)base;
 // for(int i = 0; i < argc; i++) printf("argv[%d]: %s\n", i, argv[i]);
  base += (argc + 1);
  char **envp = (char **)base;
  environ = envp;
  exit(main(argc,argv,envp));
  assert(0);
}
