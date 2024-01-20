#include <memory.h>
#include <stdint.h>
#include <proc.h>//newly added for mm_brk
static void *pf = NULL;

void* new_page(size_t nr_page) {
  pf += nr_page * PGSIZE;
  Log("free physical pages starting from %p", pf);
  return pf;
}

#ifdef HAS_VME
static void* pg_alloc(int n) {
  int nr_page = n / PGSIZE;
  assert(nr_page * PGSIZE == n);
  void *end = new_page(nr_page);
  void *start = end - n;
  memset(start, 0, n);
  return start;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}
#ifdef HAS_VME
extern PCB *current;
extern uintptr_t load_file_break;
#endif
/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
#ifdef HAS_VME
  //printf("mm_brk start allocating\n");
  if(current->max_brk == 0){
    Log("load_file_break: %p\n", (void *)load_file_break);
    void * va_base;
    if(load_file_break / PGSIZE == brk / PGSIZE){
      va_base = (void *)ROUNDUP(brk,PGSIZE);
      current->max_brk = (uintptr_t)(va_base);
    }else{
      Log("uheap_va before ROUNDDOWN: %p\n",(void *)brk);
      va_base = (void *)ROUNDDOWN(brk,PGSIZE);
      void * pa_base = pg_alloc(PGSIZE);
      map(&current->as, va_base, pa_base, 0);
      current->max_brk = (uintptr_t)(va_base + PGSIZE);
    }
    return 0;
  }
  if(brk > current->max_brk){
    unsigned int gap = brk - current->max_brk;
    unsigned int page_count = gap / PGSIZE;
    //if(page_count * PGSIZE == gap) page_count--;
    for(int i = 0; i <= page_count; i++){
      void * va_base = (void *)(current->max_brk + i * PGSIZE);
      void * pa_base = pg_alloc(PGSIZE);
      map(&current->as, va_base, pa_base, 0);
    }
    current->max_brk += (page_count + 1) * PGSIZE;
  } else {
    Log("current->max_brk: %p\n",(void *)current->max_brk);
  }
  return 0;
#else
  return 0;
#endif
}

void init_mm() {
  Log("heap.start: %p, heap.end: %p\n", heap.start, heap.end);
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
