#include <proc.h>
#include <elf.h>

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
  if(fd <= 0){
    panic("should not reach here: fd <= 0");
  }
  Elf_Ehdr elf;
  assert(fs_read(fd,&elf,sizeof(elf)) == sizeof(elf));
  //ramdisk_read(&elf,0,sizeof(Elf_Ehdr));
  assert(*(uint32_t *)elf.e_ident == 0x464c457f);
  /*Elf_Phdr phdr[elf.e_phnum];//information of the program headers
  ramdisk_read(phdr,elf.e_ehsize,elf.e_phnum * sizeof(Elf_Phdr));
  for(int i = 0; i < elf.e_phnum; i++){
    if(phdr[i].p_type == PT_LOAD){
      ramdisk_read((void *)phdr[i].p_vaddr,phdr[i].p_offset,phdr[i].p_filesz);
      memset((void *)(phdr[i].p_vaddr+phdr[i].p_filesz),0,phdr[i].p_memsz-phdr[i].p_filesz);
    }
  }*/
  Elf_Phdr phdr;
  for(int i = 0; i < elf.e_phnum; i++){
    uint32_t p_offset = elf.e_phoff + i*elf.e_phentsize;
    fs_lseek(fd,p_offset,0);
    assert(fs_read(fd,&phdr,elf.e_phentsize)==elf.e_phentsize);
    if(phdr.p_type == PT_LOAD){
      char *buf_malloc = (char *)malloc(phdr.p_filesz);
      fs_lseek(fd,phdr.p_offset,0);
      assert(fs_read(fd,buf_malloc,phdr.p_filesz)==phdr.p_filesz);
      memcpy((void*)phdr.p_vaddr,buf_malloc,phdr.p_filesz);
      memset((void*)phdr.p_vaddr + phdr.p_filesz,0,phdr.p_memsz - phdr.p_filesz);
      free(buf_malloc);
    }
  }
  //TODO();
  assert(fs_close(fd) == 0);
  return elf.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

