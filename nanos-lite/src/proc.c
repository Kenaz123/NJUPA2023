#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void naive_uload(PCB *pcb, const char *filename); 
void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    //Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (uintptr_t)arg, j);
    Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (void *)arg, j);
    j ++;
    yield();
  }
}

void init_proc() {
  switch_boot_pcb();
  Log("Initializing processes...");
  const char filename[] = "/bin/hello";
  naive_uload(NULL,filename);
  // load program here

}

Context* schedule(Context *prev) {
  return NULL;
}
