#include <am.h>
#include <nemu.h>
#include <klib.h>
#include <stdint.h>

#define VA_OFFSET(addr) (addr & 0x00000fff)
#define VA_VPN_0(addr)  ((addr >> 12) & 0x000003ff)
#define VA_VPN_1(addr)  ((addr >> 22) & 0x000003ff)

#define PA_OFFSET(addr) (addr & 0x00000fff)
#define PA_PPN(addr)    ((addr >> 12) & 0x000fffff)

static AddrSpace kas = {};
static void* (*pgalloc_usr)(int) = NULL;
static void (*pgfree_usr)(void*) = NULL;
static int vme_enable = 0;

static Area segments[] = {      // Kernel memory mappings
  NEMU_PADDR_SPACE
};

#define USER_SPACE RANGE(0x40000000, 0x80000000)

static inline void set_satp(void *pdir) {
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  return satp << 12;
}

bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc_usr = pgalloc_f;
  pgfree_usr = pgfree_f;

  kas.ptr = pgalloc_f(PGSIZE);

  int i;
  for (i = 0; i < LENGTH(segments); i ++) {
    void *va = segments[i].start;
    for (; va < segments[i].end; va += PGSIZE) {
      map(&kas, va, va, 0);
    }
  }

  set_satp(kas.ptr);
  vme_enable = 1;

  return true;
}

void protect(AddrSpace *as) {
  PTE *updir = (PTE*)(pgalloc_usr(PGSIZE));
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  memcpy(updir, kas.ptr, PGSIZE);
}

void unprotect(AddrSpace *as) {
}

void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
}

void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    set_satp(c->pdir);
  }
}

/*void map(AddrSpace *as, void *va, void *pa, int prot) {
  uintptr_t va_trans = (uintptr_t) va;
  uintptr_t pa_trans = (uintptr_t) pa;
  //assert(PA_OFFSET(pa_trans) == 0);
  //assert(VA_OFFSET(va_trans) == 0);

  uint32_t ppn = PA_PPN(pa_trans);
  uint32_t vpn_1 = VA_VPN_1(va_trans);
  uint32_t vpn_0 = VA_VPN_0(va_trans);
  //printf("[PA_PPN]: 0x%x, [VA_VPN_1]: 0x%x, [VA_VPN_0]: 0x%x\n", ppn, vpn_1, vpn_0);

  PTE * page_dir_base = (PTE *)as->ptr;
  PTE * page_dir_target = page_dir_base + vpn_1;
  //printf("[DIR BASE]: %p,[DIR TARGET]: %p\n",page_dir_base, page_dir_target);
  if(*page_dir_target == 0){//empty
    PTE * page_table_base = (PTE *)pgalloc_usr(PGSIZE);
    *page_dir_target = ((PTE)page_table_base) | PTE_V;
    PTE * page_table_target = page_table_base + vpn_0;
    *page_table_target = (ppn << 12) | PTE_V | PTE_R | PTE_W | PTE_X;
  } else {
    PTE * page_table_base = (PTE *)((*page_dir_target) & 0xfffff000);
    PTE * page_table_target = page_table_base + vpn_0;
    *page_table_target = (ppn << 12) | PTE_V | PTE_R | PTE_W | PTE_X;
  }
  //printf("[map end]\n");
}*/ 
#define VPN1(va) (((uintptr_t)va >> 22) & 0x3ff)
#define VPN0(va) (((uintptr_t)va >> 12) & 0x3ff)
#define PA_PPN_MASK(pa)  ((uintptr_t)pa & 0xfffff000)
//#define PPN0(pa) (((uintptr_t)pa >> 12) & 0x3ff)
#define PTE_PPN_MASK(pte) ((uintptr_t) pte & 0xfffff000)
#define OFFSET(addr) ((uintptr_t) addr & 0xfff)
#define VALID 0x1
#define XWR 0xe

void map(AddrSpace *as, void *va, void *pa, int prot) {
   // if(as->area != 0){
     // assert((uintptr_t)va >= (uintptr_t)as->area.start && (uintptr_t)va < (uintptr_t)as->area.end);
    //}
    PTE *pdir = as->ptr;
    assert(OFFSET(pdir) == 0);
    //printf("pdir = %x\n",pdir);
    PTE *dire = pdir + VPN1(va);
    //printf("*dire = %x\n", *dire);
    //printf("dire = %p VPN1 = %x\n", dire, VPN1(va));
    static int nr = 0;
    if (*dire == 0){
        nr++;
        PTE *pt = pgalloc_usr(PGSIZE);
        *dire = (PTE_PPN_MASK(pt)) | VALID; // 11~12 bit in pte is zero, actually they are the 33~34 bit in pa
    }
    PTE* pt = (PTE *)(PTE_PPN_MASK(*dire));
    assert(OFFSET(pt) == 0);
    PTE* pte = pt + VPN0(va);
    *pte = PA_PPN_MASK(pa) | VALID | XWR;
    assert((*pte & 0xf) == 0xf);
    //printf("nr = %d pte = %p\n", nr, pte);
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  uint32_t *end = kstack.end;
  Context *base = (Context *)(end - 36);
  base->pdir = as->ptr;
  base->mepc = (uintptr_t)entry;
  return base;
}
