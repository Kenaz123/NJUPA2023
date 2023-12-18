#include <common.h>

void do_syscall(Context *c);
static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case 1:case 4: printf("event ID = %d\n c->GPRx=%d\n", e.event,c->GPRx);break;
    case 2: do_syscall(c);break;
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
