#include "am.h"
#include "klib-macros.h"
#include "memory.h"
#include <proc.h>
#include <elf.h>
#include <stdint.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif


size_t ramdisk_read(void *buf, size_t offset, size_t len);
int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_write(int fd, const void *buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
int fs_close(int fd);
#ifdef HAS_VME
uintptr_t load_file_break;
#endif

static uintptr_t loader(PCB *pcb, const char *filename) {
  int fd = fs_open(filename, 0, 0);
  if(fd < 0){
    panic("should not reach here: fd <= 0");
  }
  Elf_Ehdr elf;
  assert(fs_read(fd,&elf,sizeof(elf)) == sizeof(elf));
  //ramdisk_read(&elf,0,sizeof(Elf_Ehdr));
  assert(*(uint32_t *)elf.e_ident == 0x464c457f);
  //Elf_Phdr phdr[elf.e_phnum];//information of the program headers
  //fs_read(fd,&phdr,elf.e_phnum * sizeof(Elf_Phdr));
  //ramdisk_read(phdr,elf.e_ehsize,elf.e_phnum * sizeof(Elf_Phdr));
 // for(int i = 0; i < elf.e_phnum; i++){
    //if(phdr[i].p_type == PT_LOAD){
      //fs_read(fd, (void *)phdr[i].p_vaddr+phdr[i].p_offset, phdr[i].p_filesz);
      //ramdisk_read((void *)phdr[i].p_vaddr,phdr[i].p_offset,phdr[i].p_filesz);
     // memset((void *)(phdr[i].p_vaddr+phdr[i].p_filesz),0,phdr[i].p_memsz-phdr[i].p_filesz);
   // }
 // }
  //Elf_Phdr phdr;
#ifdef HAS_VME
  uintptr_t code_max_page_va_base = 0;
#endif
  for(int i = 0; i < elf.e_phnum; i++){
    Elf_Phdr phdr;
    uint32_t p_offset = elf.e_phoff + i*elf.e_phentsize;
    //printf("p_offset: %d\n",p_offset);
    fs_lseek(fd,p_offset,0);
    fs_read(fd,&phdr,elf.e_phentsize);
    //assert(fs_read(fd,&phdr,elf.e_phentsize)==elf.e_phentsize);
    if(phdr.p_type == PT_LOAD){
      uintptr_t offset = phdr.p_offset;
      uintptr_t virtAddr = phdr.p_vaddr;
      uintptr_t fileSiz = phdr.p_filesz;
      uintptr_t memSiz = phdr.p_memsz;
      uintptr_t flags = phdr.p_flags;
#ifdef HAS_VME
      if(flags == (PF_R | PF_X)){//code
        uintptr_t code_break = virtAddr + memSiz;
        if(code_break % PGSIZE == 0) code_max_page_va_base = code_break - PGSIZE;
        else code_max_page_va_base = (uintptr_t) ROUNDDOWN(code_break, PGSIZE); 
      }
      if(flags == (PF_R | PF_W)){//data
        load_file_break = virtAddr + memSiz;//check again plz
        if(load_file_break > pcb->max_brk){
          pcb->max_brk = load_file_break;
        }
        assert(code_max_page_va_base != 0);
        assert(virtAddr > code_max_page_va_base);
        Log("code_max_page_va_base: %d,data_page_va_base: %d\n",code_max_page_va_base,virtAddr);
      }
      if(virtAddr % PGSIZE == 0){
        unsigned int page_count = memSiz / PGSIZE;
        if(page_count * PGSIZE == memSiz) page_count--;
        unsigned int page_count_file = fileSiz / PGSIZE;
        if(page_count_file * PGSIZE == fileSiz) page_count--;

        for(unsigned int i = 0; i <= page_count_file; i++){
          size_t read_bytes = PGSIZE;
          if(i == page_count_file) read_bytes = (fileSiz % PGSIZE == 0) ? PGSIZE : (fileSiz % PGSIZE);
          void * paddr = new_page(1);
          paddr -= PGSIZE;
          memset(paddr, 0, PGSIZE);
          void * vaddr = (void *)(virtAddr + i * PGSIZE);
          assert(fs_lseek(fd, offset + i * PGSIZE, 0) >= 0);
          assert(fs_read(fd,paddr,read_bytes) >= 0);

          map(&pcb->as, vaddr, paddr, 0);
        }
        if(page_count > page_count_file){
          for(int i = page_count_file + 1; i <= page_count; i++){
            void * paddr = new_page(1);
            paddr -= PGSIZE;
            memset(paddr, 0, PGSIZE);
            void * vaddr = (void *)(virtAddr + i * PGSIZE);
            map(&pcb->as, vaddr, paddr, 0);
          }
        }
      } else {
        uintptr_t gap = virtAddr - (uintptr_t)ROUNDDOWN(virtAddr, PGSIZE);
        unsigned int page_count = (gap + memSiz) / PGSIZE;
        if(page_count * PGSIZE == (gap + memSiz)) page_count--;
        unsigned int page_count_file = (gap + fileSiz) / PGSIZE;
        if(page_count_file * PGSIZE == (gap + fileSiz)) page_count--;

        for(int i = 0; i <= page_count_file; i++){
          if(i == 0){
            size_t read_bytes = PGSIZE - gap;
            void * paddr = new_page(1);
            paddr -= PGSIZE;
            memset(paddr, 0, PGSIZE);

            void * vaddr = (void *)(virtAddr - gap + i * PGSIZE);
            assert(fs_lseek(fd, offset + i * PGSIZE, 0) >= 0);
            assert(fs_read(fd, paddr + gap, read_bytes) >= 0);
            map(&pcb->as, vaddr, paddr, 0);
          } else {
            size_t read_bytes = PGSIZE;
            if(i == page_count_file) read_bytes = ((gap + fileSiz) % PGSIZE == 0) ? PGSIZE : ((gap + fileSiz) % PGSIZE);
            void * paddr = new_page(1);
            paddr -= PGSIZE;
            memset(paddr, 0, PGSIZE);

            void * vaddr = (void *)(virtAddr - gap + i * PGSIZE);
            assert(fs_lseek(fd, offset + (i - 1) * PGSIZE + (PGSIZE - gap), 0) >= 0);
            assert(fs_read(fd, paddr, read_bytes) >= 0);
            map(&pcb->as, vaddr, paddr, 0);
          }
        }
        if(page_count > page_count_file){
          for(int i = page_count_file + 1; i <= page_count; i++){
            void * paddr = new_page(1);
            paddr -= PGSIZE;
            memset(paddr, 0, PGSIZE);
            void * vaddr = (void *)(virtAddr - gap + i * PGSIZE);
            map(&pcb->as, vaddr, paddr, 0);
          }
        }
      }
#else
      char *buffer = (char *)malloc(phdr.p_filesz * sizeof(char) + 1);
      fs_lseek(fd,phdr.p_offset,0);
      fs_read(fd,buffer,phdr.p_filesz);
      memcpy((void*)(uintptr_t)phdr.p_vaddr,buffer,phdr.p_filesz);
      memset((void*)(uintptr_t)(phdr.p_vaddr + phdr.p_filesz),0,phdr.p_memsz - phdr.p_filesz);
      free(buffer);

#endif
    }
  }
  //TODO();
  assert(fs_close(fd) == 0);
  return elf.e_entry;
}
/*#define min(x, y) ((x < y) ? x : y)
#define PG_MASK (~0xfff)
#define ISALIGN(vaddr) ((vaddr) == ((vaddr)&PG_MASK))
#define OFFSET(vaddr) ((vaddr) & (~PG_MASK))
#define NEXT_PAGE(vaddr) ((ISALIGN(vaddr)) ? (vaddr) : ((vaddr)&PG_MASK) + PGSIZE)
static uintptr_t loader(PCB *pcb, const char *filename)
{
  Elf_Ehdr ehdr;

  // ramdisk_read(&ehdr, 0, sizeof(ehdr));
  int fd = fs_open(filename, 0, 0);
  assert(fd != -1);

  fs_read(fd, &ehdr, sizeof(ehdr));

  char riscv32_magic_num[] = {0x7f, 0x45, 0x4c, 0x46, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  // printf("magic number is %s\n", (char *)(ehdr.e_ident));
  assert(strcmp((char *)(ehdr.e_ident), riscv32_magic_num) == 0);

  uint32_t entry = ehdr.e_entry;
  uint32_t ph_offset = ehdr.e_phoff;
  uint32_t ph_num = ehdr.e_phnum;

  Elf_Phdr phdr;
  for (int i = 0; i < ph_num; ++i)
  {
    // ramdisk_read(&phdr, ph_offset + i * sizeof(phdr), sizeof(phdr));
    fs_lseek(fd, ph_offset + i * sizeof(phdr), 0);
    fs_read(fd, &phdr, sizeof(phdr));
    if (phdr.p_type != PT_LOAD)
      continue;

    // printf("load program header %d", i);

    uint32_t offset = phdr.p_offset;
    uint32_t file_size = phdr.p_filesz;
    uint32_t p_vaddr = phdr.p_vaddr;
    uint32_t mem_size = phdr.p_memsz;

    printf("load program from [%p, %p] to [%p, %p]\n", offset, file_size, p_vaddr, mem_size);
#ifdef HAS_VME
    int left_size = file_size;
    fs_lseek(fd, offset, 0);
    // printf("vaddr is %p\n", p_vaddr);
    if (!ISALIGN(p_vaddr))
    {
      void *pg_p = new_page(1);
      int read_len = min(PGSIZE - OFFSET(p_vaddr), left_size);
      left_size -= read_len;
      assert(fs_read(fd, pg_p + OFFSET(p_vaddr), read_len) >= 0);
      map(&pcb->as, (void *)p_vaddr, pg_p, 0);
      p_vaddr += read_len;
    }

    for (; p_vaddr < phdr.p_vaddr + file_size; p_vaddr += PGSIZE)
    {
      assert(ISALIGN(p_vaddr));
      void *pg_p = new_page(1);
      memset(pg_p, 0, PGSIZE);
      // int len = min(PGSIZE, file_size - fs_lseek(fd, 0, SEEK_CUR));
      int read_len = min(PGSIZE, left_size);
      left_size -= read_len;
      assert(fs_read(fd, pg_p, read_len) >= 0);
      map(&pcb->as, (void *)p_vaddr, pg_p, 0);
    }
    // printf("p_vaddr is %p\n", (void *)p_vaddr);
    p_vaddr = NEXT_PAGE(p_vaddr);
    printf("p_vaddr is %p next page, end of uninitialized space is %p\n", (void *)p_vaddr, (void *)(phdr.p_vaddr + mem_size));
    for (; p_vaddr < phdr.p_vaddr + mem_size; p_vaddr += PGSIZE)
    {
      assert(ISALIGN(p_vaddr));
      void *pg_p = new_page(1);
      memset(pg_p, 0, PGSIZE);
      map(&pcb->as, (void *)p_vaddr, pg_p, 0);
    }
#else
    // ramdisk_read((void *)vaddr, offset, file_size);
    fs_lseek(fd, offset, 0);
    fs_read(fd, (void *)p_vaddr, file_size);
    memset((void *)(p_vaddr + file_size), 0, mem_size - file_size);
#endif
    assert(mem_size >= file_size);
  }

  // printf("max brk is at %p when load\n", pcb->max_brk);
  assert(fs_close(fd) != -1);

  return entry;
}*/

/*static uintptr_t loader(PCB *pcb, const char *filename) {
  Elf_Ehdr header;
  int fd = fs_open(filename, 0, 0);
  assert(fd >= 0);
  fs_read(fd ,&header,sizeof(header));
  assert(*(uint32_t *)header.e_ident == 0x464c457f);
  //Elf_Addr entry = header.e_entry;
  //Elf_Off  phoff = header.e_phoff;
  //Elf_Half phentsize = header.e_phentsize;
  assert(sizeof(Elf_Phdr) == header.e_phentsize);
  //Elf_Half phnum = header.e_phnum;
  //assert(phnum!=0);
  Elf_Phdr pheader;
  for(int i = 0; i < header.e_phnum; i++){
      fs_lseek(fd, header.e_phoff + i * header.e_phentsize, 0);
      fs_read(fd, &pheader, header.e_phentsize);
      //ramdisk_read(&pheader,phoff + i*phentsize ,phentsize);
      if(pheader.p_type == PT_LOAD){
          uintptr_t filesz = pheader.p_filesz;
          uintptr_t vaddr  = pheader.p_vaddr;
          //assert((vaddr & 0xfff) == 0);
          uintptr_t  offset = pheader.p_offset;
          uintptr_t memsz  = pheader.p_memsz;

          assert(memsz >= filesz);
          uintptr_t end = vaddr + memsz;
          if(end > pcb->max_brk){
              pcb->max_brk = end;
          }

          //uintptr_t head = 0;
          if((vaddr & 0xfff) != 0){
              void *pa = (void *)((uintptr_t)new_page(1) + (vaddr & 0xfff));
              void *va = (void *)vaddr;
              map(&(pcb->as), va, pa, 0);
              fs_lseek(fd, offset, 0);
              uintptr_t hsize = PGSIZE - (vaddr & 0xfff);
              fs_read(fd, pa, hsize);
              offset = offset + hsize;
              filesz = filesz - hsize;
              memsz  = memsz  - hsize;
              vaddr  = vaddr  + hsize;
          }
          assert((vaddr & 0xfff) == 0);

          int nr_page = ROUNDUP(filesz, PGSIZE) / PGSIZE;
          int j = 0;
          for(j = 0; j < nr_page; j++){
              void *pa = new_page(1);
              void *va = (void *)(vaddr + j * PGSIZE);
              map(&(pcb->as), va, pa, 0);
              uintptr_t pg_offset = offset + j * PGSIZE;
              fs_lseek(fd, pg_offset, 0);
              if(j != nr_page -1){
                  fs_read(fd, pa, PGSIZE);
              }
              else{
                  uint32_t rest = filesz - j * PGSIZE;
                  assert(rest > 0 && rest <= PGSIZE);
                  fs_read(fd, pa, rest);
                  uint32_t bss = memsz - filesz;
                  if(bss + rest <= PGSIZE){
                      memset((void *)((uintptr_t)pa + rest), 0, bss);
                  }
                  else{
                      //assert(0);
                      memset((void *)((uintptr_t)pa + rest), 0, PGSIZE - rest);
                      bss = bss - (PGSIZE - rest);
                      int bss_nr_page = ROUNDUP(bss, PGSIZE) / PGSIZE;
                      //printf("bss_nr_page = %d\n", bss_nr_page);
                      for(int k = 0; k < bss_nr_page; k++){
                          void * bss_pa = new_page(1);
                          //printf("bss_pa = %x\n", bss_pa);
                          void * bss_va = (void *)(vaddr + (nr_page + k) * PGSIZE);
                          map(&(pcb->as), bss_va, bss_pa, 0);
                          if(k != bss_nr_page - 1){
                              memset(bss_pa, 0, PGSIZE);
                          }
                          else{
                              memset(bss_pa, 0, bss & 0xfff);
                          }
                      }
                  }
              }
              //memset((void *)(vaddr + filesz),0,memsz-filesz);
          }
          
      }
  }
  fs_close(fd); 
  return header.e_entry;
}*/

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", (void *)entry);
  ((void(*)())entry) ();
}

void context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
  Area stack;
  stack.start = pcb->stack;
  stack.end = pcb->stack + STACK_SIZE;
  pcb->cp = kcontext(stack, entry, arg);
}

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
//  uintptr_t entry = loader(pcb,filename);
  protect(&(pcb->as));
  Area stack;
  stack.start = pcb->stack;
  stack.end = pcb->stack + STACK_SIZE;
  //uint8_t *end = heap.end;
  //stack.end = end;
  //stack.start = end - STACK_SIZE;
//  pcb->cp = ucontext(NULL,stack,(void(*)())entry);
  //pcb->cp->GPRx = (uintptr_t)heap.end;
  void *ustack_end = new_page(8);
#ifdef HAS_VME
  void *ustack_start = ustack_end - 8 * PGSIZE;
  void *ustack_start_vaddr = pcb->as.area.end - 8 * PGSIZE;
  for(int i = 0; i < 8; i++) map(&pcb->as, ustack_start_vaddr + i * PGSIZE, ustack_start + i * PGSIZE, 0);
#endif
  int space_count = 0;
  int argc = 0;
  if(argv) while(argv[argc]){
    argc++;
  }
  space_count += sizeof(uintptr_t);
  space_count += sizeof(uintptr_t) * (argc + 1);
  if(argv) for(int i = 0; i < argc; ++i) space_count += (strlen(argv[i])+1);

  int envpc = 0;
  if(envp) while(envp[envpc]) envpc++;
  space_count += sizeof(uintptr_t) * (envpc + 1);
  if(envp) for(int i = 0; i < envpc; ++i) space_count += (strlen(envp[i]) + 1);

  space_count += sizeof(uintptr_t);
  uintptr_t *base = (uintptr_t *)ROUNDUP(ustack_end - space_count,sizeof(uintptr_t));
  uintptr_t *base_mem = base;
  *base = argc;
  base += 1;

  char *argv_tmp[argc];
  char *envp_tmp[envpc];
  base += (argc + 1) + (envpc + 1);
  char *string_area = (char *)base;
  uintptr_t *string_area_mem = (uintptr_t *)string_area;
  for(int i = 0; i < argc; i++){
    strcpy(string_area, argv[i]);
    argv_tmp[i] = string_area;
    string_area += (strlen(argv[i])+1);
  }

  for(int i = 0; i < envpc; i++){
    strcpy(string_area,envp[i]);
    envp_tmp[i] = string_area;
    string_area += (strlen(envp[i])+1);
  }
  base -= (argc + 1) + (envpc + 1);
  
  for(int i = 0; i < argc; i++){
    *base = (uintptr_t)argv_tmp[i];
    base += 1;
  }
  *base = (uintptr_t)NULL;
  base += 1;
  for(int i = 0; i < envpc; i++){
    *base = (uintptr_t)envp_tmp[i];
    base += 1;
  }
  *base = (uintptr_t)NULL;
  base += 1;
  assert(string_area_mem == base);
  uintptr_t entry = loader(pcb, filename);
  //printf("finish loader\n");
  pcb->cp = ucontext(&pcb->as, stack, (void(*)())entry);
  Log("entry:%p",(void *)entry);
  pcb->cp->GPRx = (uintptr_t)base_mem;


}

