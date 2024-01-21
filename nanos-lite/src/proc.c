#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;
PCB *fg_pcb = NULL;
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

static uint64_t vttime[2] = {15, 1};
static uint64_t duration[2] = {0};
static uint32_t cur_pcb_id = 0;
#define argmin(a, b) (((a) < (b)) ? 0 : 1)

void init_proc() {
  char *argv_nterm[] = {"/bin/nterm", NULL};
  char *argv_hello[] = {"/bin/hello", NULL};
  char *envp[] = {NULL};
  //context_kload(&pcb[1], hello_fun, "A");
  context_uload(&pcb[1], "/bin/nterm", argv_nterm, envp);
  context_uload(&pcb[2], "/bin/nterm", argv_nterm, envp);
  context_uload(&pcb[3], "/bin/nterm", argv_nterm, envp);
  context_uload(&pcb[0], "/bin/hello", argv_hello, envp);
  //context_kload(&pcb[1], hello_fun, "B");
  fg_pcb = &pcb[1];
  switch_boot_pcb();
  //Log("Initializing processes...");
  //const char filename[] = "/bin/nterm";
  //naive_uload(NULL,filename);
  // load program here

}

Context* schedule(Context *prev) {
  current->cp = prev;
  duration[cur_pcb_id] += vttime[cur_pcb_id];
  cur_pcb_id = argmin(duration[0], duration[1]);
  current = (cur_pcb_id == 0) ? &pcb[0] : fg_pcb;
  //current = ((current == &pcb[0]) ? &pcb[1] : &pcb[0]);
  return current->cp;
}

void set_fg_pcb(uint32_t process_id)
{
  fg_pcb = &pcb[process_id];
  if (duration[1] < duration[0])
    duration[1] = duration[0];
}
