/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <memory/host.h>
#include <memory/paddr.h>
#include <device/mmio.h>
#include <isa.h>

#if   defined(CONFIG_PMEM_MALLOC)
static uint8_t *pmem = NULL;
#else // CONFIG_PMEM_GARRAY
static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};
#endif

#ifdef CONFIG_MTRACE_COND
static char mtrace[128];
static const char Memory_type[2][3]={"Mw","Mr"};
#endif
#ifdef CONFIG_MTRACE
static void memory_trace(paddr_t addr, int len, int data, int flag){
  strncpy(mtrace,Memory_type[flag],2);
  memset(mtrace + 2,' ',4);
  char *p = mtrace + 6;
  p += snprintf(p,sizeof(mtrace) - 6,FMT_WORD "    ", addr);
  p += snprintf(p,mtrace + sizeof(mtrace) - p,"%2d""    ", len);
  int i;
  uint8_t *data_ptr = (uint8_t *)&data;
  for(i = len - 1; i >= 0; i--){
    p += snprintf(p,4," %02x", data_ptr[i]);
  }
  *p = '\0';
#ifdef CONFIG_MTRACE_COND
  if(MTRACE_COND) {log_write("%s\n", mtrace);}
#endif
  puts(mtrace);
} 
#endif

uint8_t* guest_to_host(paddr_t paddr) { return pmem + paddr - CONFIG_MBASE; }
paddr_t host_to_guest(uint8_t *haddr) { return haddr - pmem + CONFIG_MBASE; }

static word_t pmem_read(paddr_t addr, int len) {
  word_t ret = host_read(guest_to_host(addr), len);
  return ret;
}

static void pmem_write(paddr_t addr, int len, word_t data) {
  host_write(guest_to_host(addr), len, data);
}

static void out_of_bound(paddr_t addr) {
  panic("address = " FMT_PADDR " is out of bound of pmem [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
      addr, PMEM_LEFT, PMEM_RIGHT, cpu.pc);
}

void init_mem() {
#if   defined(CONFIG_PMEM_MALLOC)
  pmem = malloc(CONFIG_MSIZE);
  assert(pmem);
#endif
  IFDEF(CONFIG_MEM_RANDOM, memset(pmem, rand(), CONFIG_MSIZE));
  Log("physical memory area [" FMT_PADDR ", " FMT_PADDR "]", PMEM_LEFT, PMEM_RIGHT);
}

word_t paddr_read(paddr_t addr, int len) {
  if (likely(in_pmem(addr))) {
#ifdef CONFIG_MTRACE
      word_t data = pmem_read(addr,len);
      memory_trace(addr,len,data,1);
      return data;
#else
    return pmem_read(addr, len);
#endif
  }
  IFDEF(CONFIG_DEVICE, IFDEF(CONFIG_MTRACE,word_t data = mmio_read(addr,len);memory_trace(addr,len,data,1);return data;)return mmio_read(addr, len));
#ifdef CONFIG_MTRACE
  memory_trace(addr,len,-1,1);
#endif
  out_of_bound(addr);
  return 0;
}

void paddr_write(paddr_t addr, int len, word_t data) {
#ifdef CONFIG_MTRACE
  memory_trace(addr,len,data,0);
#endif
  if (likely(in_pmem(addr))) { pmem_write(addr, len, data); return; }
  IFDEF(CONFIG_DEVICE, mmio_write(addr, len, data); return);
  out_of_bound(addr);
}
