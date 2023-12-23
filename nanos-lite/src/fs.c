#include "klib-macros.h"
#include <fs.h>
#include <stddef.h>

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

typedef struct {
  size_t fd;
  size_t open_offset;
} OFinfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, DEV_EVENTS, PROC_DISPINFO, FD_FB};
size_t serial_write(const void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len);
size_t dispinfo_read(void *buf, size_t offset, size_t len);

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write},
  [DEV_EVENTS] = {"/dev/events", 0, 0, events_read, invalid_write},
  [PROC_DISPINFO] = {"/proc/dispinfo", 0, 0, dispinfo_read, invalid_write},
#include "files.h"
};

static OFinfo open_file_table[LENGTH(file_table)];
static size_t open_file_index = 0;


void init_fs() {
  AM_GPU_CONFIG_T ev = io_read(AM_GPU_CONFIG);
  int width = ev.width;
  int height = ev.height;
  file_table[FD_FB].size = width * height * sizeof(uint32_t);
  // TODO: initialize the size of /dev/fb
}

int fs_open(const char *pathname, int flags, int mode){
    for(int i = 0; i < LENGTH(file_table); i++){
      if(strcmp(file_table[i].name,pathname)==0){
        if(i < FD_FB){
          //Log("ignore opening %s",pathname);
          return i;
        }
        open_file_table[open_file_index].fd = i;
        open_file_table[open_file_index].open_offset = 0;
        open_file_index++;
        return i;
      }
    }
    panic("file %s not found",pathname);
}

static int get_open_index(int fd){
  for(int i = 0; i < open_file_index; i++){
    if(open_file_table[i].fd == fd){
      return i;
    }
    else continue;
  }
  return -1;//for failure
}

size_t fs_read(int fd, void *buf, size_t len){
  ReadFn readfn = file_table[fd].read;
  if(readfn != NULL){
      return readfn(buf,0,len);
    }
  /*if(fd <= 2){
    Log("ignore reading %s",file_table[fd].name);
    return 0; 
  }*/
  int target_read = get_open_index(fd);
  if(target_read == -1){
      Log("file %s not opened before read",file_table[fd].name);
      return -1;
  }

  size_t read_byte = len;
  size_t cur_open_offset = open_file_table[target_read].open_offset;
  size_t size = file_table[fd].size;
  size_t disk_offset = file_table[fd].disk_offset;
  if(cur_open_offset > size) return 0;
  if(cur_open_offset + len > size) read_byte = size - cur_open_offset;
  ramdisk_read(buf, disk_offset+cur_open_offset, read_byte);
  open_file_table[target_read].open_offset += read_byte;
  return read_byte;
}

size_t fs_write(int fd, const void *buf, size_t len){
  if(fd == 0){
    Log("ignore writing %s",file_table[fd].name);
    return 0; 
  }
  WriteFn writefn = file_table[fd].write;
  if(writefn != NULL) {
    return writefn(buf,0,len);
  }
  /*if(fd == 1 || fd == 2){
    for(int i = 0; i < len; i++){
        putch(*((char*)buf + i));
    }
    return len;
  }//pay attention to Stdout and Stderr!!!
  */
  int target_write = get_open_index(fd);
  if(target_write == -1){
      Log("file %s not opened before write",file_table[fd].name);
      return -1;
  }
  size_t write_byte = len;
  size_t cur_open_offset = open_file_table[target_write].open_offset;
  size_t size = file_table[fd].size;
  size_t disk_offset = file_table[fd].disk_offset;
  if(cur_open_offset > size) return 0;
  if(cur_open_offset + len > size) write_byte = size - cur_open_offset;
  ramdisk_write(buf, disk_offset+cur_open_offset, write_byte);
  open_file_table[target_write].open_offset += write_byte;
  return write_byte;
}

size_t fs_lseek(int fd, size_t offset, int whence){
  if(fd <= 2){
      Log("ignore lseek %s",file_table[fd].name);
      return 0; 
  }
  int target_lseek = get_open_index(fd);
  if(target_lseek == -1){
      Log("file %s not opened before lseek",file_table[fd].name);
      return (long int)-1;
  }
  size_t new_offset = -1;
  size_t size = file_table[fd].size;
  size_t open_offset = open_file_table[target_lseek].open_offset;
  switch(whence) {
      case SEEK_SET:
        if(offset>size) new_offset = size;
        new_offset = offset;break;
      case SEEK_CUR:
        if(offset+open_offset>size) new_offset=size;
        new_offset = offset + open_offset;break;
      case SEEK_END:
        if(offset+size>size) new_offset = size;
        new_offset = offset + size;break;
      default:panic("Failure during fs_lseek : unhandled whence %d",whence);
  }
  if(new_offset<0||new_offset>size){
      Log("Seek position out of bounds");
      return -1;
    }
  open_file_table[target_lseek].open_offset = new_offset;
  return new_offset;
}

int fs_close(int fd){
  if(fd <= 2){
      //Log("ignore close %s",file_table[fd].name);
      return 0;
  }
  int target_close = get_open_index(fd);
  if(target_close >= 0){
      for(int i = target_close; i < open_file_index - 1; i++){
        open_file_table[i] = open_file_table[i+1];
      }
      open_file_index--;
      return 0;
  }
  //Log("file not opened before close");
  return 0;
}
