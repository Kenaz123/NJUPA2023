#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>
#include <stdint.h>

static Context* (*user_handler)(Event, Context*) = NULL;
void __am_get_cur_as(Context *c);
void __am_switch(Context *c);
Context* __am_irq_handle(Context *c) {
  __am_get_cur_as(c);
  if (user_handler) {
    Event ev = {0};
    //printf("c->mcause=%d\n",c->mcause);
    c->mepc+=4;
    switch (c->mcause) {
      case -1: ev.event = EVENT_YIELD; break;
      case 1:case 4: ev.event = EVENT_SYSCALL; break;
      case EVENT_IRQ_TIMER: ev.event = EVENT_IRQ_TIMER; break;
      default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }
  __am_switch(c);
  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  uint32_t *end = kstack.end;
  Context *base = (Context *)(end - 36);//32+3+1=36
  base->pdir = NULL;
  base->mepc = (uintptr_t)entry;
  base->mstatus = 1 << 7;
  base->gpr[10] = (uintptr_t)arg;
  return base;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
