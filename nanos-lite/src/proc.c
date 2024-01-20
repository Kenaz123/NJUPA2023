#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void naive_uload(PCB *pcb, const char *filename);

void context_kload(PCB *pcb, void (*entry)(void *), void *arg);
//void context_uload(PCB *pcb, const char *filename);
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);
void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    //Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (uintptr_t)arg, j);
    //Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (void *)arg, j);
   // Log("Hello World from Nanos-lite with arg '%s' for the %dth time!", (const char*)arg, j);
    j++;
    yield();
  }
}

void init_proc() {
  char *argv_nterm[] = {"/bin/menu", NULL};
  char *argv_hello[] = {"/bin/hello", NULL};
  char *envp[] = {NULL};
  //context_kload(&pcb[1], hello_fun, "A");
  context_uload(&pcb[0], "/bin/menu", argv_nterm, envp);
  context_uload(&pcb[1], "/bin/hello", argv_hello, envp);
  //context_kload(&pcb[1], hello_fun, "B");
  switch_boot_pcb();
  //Log("Initializing processes...");
  //const char filename[] = "/bin/nterm";
  //naive_uload(NULL,filename);
  // load program here

}

Context* schedule(Context *prev) {
  current->cp = prev;
  current = ((current == &pcb[0]) ? &pcb[1] : &pcb[0]);
  return current->cp;
}
