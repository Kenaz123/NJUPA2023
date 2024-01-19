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

#include "common.h"
#include <isa.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>
#include <stdint.h>
/*#define VA_OFFSET(addr) (addr & 0x00000fff)
#define VA_VPN_0(addr)  ((addr >> 12) & 0x000003ff)
#define VA_VPN_1(addr)  ((addr >> 22) & 0x000003ff)

#define PTE_V(item)   (item & 0x1)
#define PTE_R(item)   ((item >> 1) & 0x1)
#define PTE_W(item)   ((item >> 2) & 0x1)
#define PTE_X(item)   ((item >> 3) & 0x1)
#define PTE_PPN(item) ((item >> 12) & 0xfffff)

typedef vaddr_t PTE;

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  //Log("[VA]: 0x%x", vaddr);
  word_t satp = cpu.satp;
  PTE page_dir_base = satp << 12;

  uint32_t offset = VA_OFFSET(vaddr);
  uint32_t vpn_1 = VA_VPN_1(vaddr);
  uint32_t vpn_0 = VA_VPN_0(vaddr);

  PTE page_dir_target = page_dir_base + vpn_1 * 4;
  word_t page_dir_target_item = paddr_read(page_dir_target, 4);
  if(PTE_V(page_dir_target_item) == 0) assert(0);

  PTE page_table_base = PTE_PPN(page_dir_target_item) << 12;
  PTE page_table_target = page_table_base + vpn_0 * 4;
  word_t page_table_target_item = paddr_read(page_table_target, 4); 
 
  if(PTE_V(page_table_target_item) == 0){ 
    Log("[VA]: 0x%x, [Table Target]: 0x%x, [Table Target Item]: 0x%x\n",vaddr, page_table_target, page_table_target_item);
    assert(0);
  }

  switch(type){
    case MEM_TYPE_IFETCH: if(PTE_X(page_table_target_item) == 0) assert(0);
      break;
    case MEM_TYPE_READ: if(PTE_R(page_table_target_item) == 0) assert(0);
      break;
    case MEM_TYPE_WRITE: if(PTE_W(page_table_target_item) == 0) assert(0);
      break;
    default: assert(0); break;
  }
  paddr_t ppn = PTE_PPN(page_table_target_item) << 12;
  paddr_t paddr = ppn | offset;
  //assert(paddr == vaddr);
  return paddr;
}*/
#define VPN1(va) (((uintptr_t)va >> 22) & 0x3ff)
#define VPN0(va) (((uintptr_t)va >> 12) & 0x3ff)
#define PA_PPN_MASK(pa)  ((uintptr_t)pa & 0xfffff000)
//#define PPN0(pa) (((uintptr_t)pa >> 12) & 0x3ff)
#define PTE_PPN_MASK(pte) ((uintptr_t) pte & 0xfffff000)
#define OFFSET(addr) ((uintptr_t) addr & 0xfff)
#define VALID 0x1
#define XWR 0xe
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  paddr_t pdir = cpu.satp << 12; //csr[4] = satp
  paddr_t dire_addr = pdir + VPN1(vaddr) * sizeof(word_t);
  word_t dire = paddr_read(dire_addr, sizeof(word_t));
  if((dire & 0xf) != 0x1){
      printf("vaddr = %x\n", vaddr);
  }

  assert((dire & 0xf) == 0x1);

  paddr_t pt = PTE_PPN_MASK(dire);
  paddr_t pte_addr = pt + VPN0(vaddr) * sizeof(word_t);
  word_t pte = paddr_read(pte_addr, sizeof(word_t));
  
  if((pte & 0xf) != 0xf){
      printf("vaddr = %x\n", vaddr);
  }
  assert((pte & 0xf) == 0xf);

  paddr_t paddr = PTE_PPN_MASK(pte) + OFFSET(vaddr);
  //assert( paddr == vaddr );
  return paddr;
}
