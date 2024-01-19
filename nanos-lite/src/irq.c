#include <common.h>
Context* schedule(Context *prev); 
void do_syscall(Context *c);
static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_YIELD: 
      //printf("event ID = %d\n c->GPRx=%d\n", e.event,c->GPRx);
      //halt(0);
      c = schedule(c);
      break;
    case EVENT_SYSCALL:case 4: do_syscall(c);
      break;
    case EVENT_IRQ_TIMER: Log("EVENT_IRQ_TIMER\n");
      c = schedule(c);
      break;
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
