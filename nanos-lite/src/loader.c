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

static uintptr_t loader(PCB *pcb, const char *filename) {
  int fd = fs_open(filename, 0, 0);
  if(fd < 0){
    panic("should not reach here: fd <= 0");
  }
  Elf_Ehdr elf;
  assert(fs_read(fd,&elf,sizeof(elf)) == sizeof(elf));
  //ramdisk_read(&elf,0,sizeof(Elf_Ehdr));
  assert(*(uint32_t *)elf.e_ident == 0x464c457f);
  /*Elf_Phdr phdr[elf.e_phnum];//information of the program headers
  fs_read(fd,&phdr,elf.e_phnum * sizeof(Elf_Phdr));
  //ramdisk_read(phdr,elf.e_ehsize,elf.e_phnum * sizeof(Elf_Phdr));
  for(int i = 0; i < elf.e_phnum; i++){
    if(phdr[i].p_type == PT_LOAD){
      //fs_read(fd, (void *)phdr[i].p_vaddr+phdr[i].p_offset, phdr[i].p_filesz);
      ramdisk_read((void *)phdr[i].p_vaddr,phdr[i].p_offset,phdr[i].p_filesz);
      memset((void *)(phdr[i].p_vaddr+phdr[i].p_filesz),0,phdr[i].p_memsz-phdr[i].p_filesz);
    }
  }*/
  //Elf_Phdr phdr;
  for(int i = 0; i < elf.e_phnum; i++){
    Elf_Phdr phdr;
    uint32_t p_offset = elf.e_phoff + i*elf.e_phentsize;
    printf("p_offset: %d\n",p_offset);
    fs_lseek(fd,p_offset,0);
    fs_read(fd,&phdr,elf.e_phentsize);
    //assert(fs_read(fd,&phdr,elf.e_phentsize)==elf.e_phentsize);
    if(phdr.p_type == PT_LOAD){
      char *buffer = (char *)malloc(phdr.p_filesz * sizeof(char) + 1);
      //printf("phdr_p_offset:%d\n",phdr.p_offset);
      fs_lseek(fd,phdr.p_offset,0);
      fs_read(fd,buffer,phdr.p_filesz);
      memcpy((void*)(uintptr_t)phdr.p_vaddr,buffer,phdr.p_filesz);
      memset((void*)(uintptr_t)(phdr.p_vaddr + phdr.p_filesz),0,phdr.p_memsz - phdr.p_filesz);
      free(buffer);
    }
  }
  //TODO();
  assert(fs_close(fd) == 0);
  return elf.e_entry;
}

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
  Area stack;
  stack.start = pcb->stack;
  stack.end = pcb->stack + STACK_SIZE;
  //uint8_t *end = heap.end;
  //stack.end = end;
  //stack.start = end - STACK_SIZE;
//  pcb->cp = ucontext(NULL,stack,(void(*)())entry);
  //pcb->cp->GPRx = (uintptr_t)heap.end;
  void *ustack_end = new_page(8);
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
  pcb->cp = ucontext(NULL, stack, (void(*)())entry);
  Log("entry:%d",(void *)entry);
  pcb->cp->GPRx = (uintptr_t)base_mem;


}

