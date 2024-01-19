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
#define VA_OFFSET(addr) (addr & 0x00000fff)
#define VA_VPN_0(addr)  ((addr >> 12) & 0x000003ff)
#define VA_VPN_1(addr)  ((addr >> 22) & 0x000003ff)

#define PTE_V(item)   (item & 0x1)
#define PTE_R(item)   ((item >> 1) & 0x1)
#define PTE_W(item)   ((item >> 2) & 0x1)
#define PTE_X(item)   ((item >> 3) & 0x1)
#define PTE_PPN(item) ((item >> 12) & 0xfffff)

typedef vaddr_t PTE;

/*paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
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
    Log("[Table Target]: 0x%x, [Table Target Item]: 0x%x\n",page_table_target, page_table_target_item);
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
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type)
{
  // // {
  // printf("================\n");
  // }
#define SATP_MASK 0X3fffff
#define PG_SHIFT 12
#define PGT1_ID(val) (val >> 22)
#define PGT2_ID(val) ((val & 0x3fffff) >> 12)
#define OFFSET(val) (val & 0xfff)
  word_t va_raw = (uint32_t)vaddr;
  paddr_t *pt_1 = (paddr_t *)guest_to_host((paddr_t)(cpu.satp << PG_SHIFT));
  // printf("pt1[id1(vaddr)] is 0x%x\n", pt_1[PGT1_ID(va_raw)]);
  assert(pt_1 != NULL);
  word_t *pt_2 = (word_t *)guest_to_host(pt_1[PGT1_ID(va_raw)]);
  // if (vaddr == 0x7ffffded)

  assert(pt_2 != NULL);
  paddr_t paddr = (paddr_t)((pt_2[PGT2_ID(va_raw)] & (~0xfff)) | OFFSET(va_raw));
  // if (vaddr >= 0x40000000 && vaddr <= 0x8000000)
  // {
  // printf("mmu translate vaddr %p\n", (void *)(long)vaddr);
  // printf("translate ui is 0x%x, pt2 id is %x\n", pt_2[PGT2_ID(va_raw)] >> 12, PGT2_ID(va_raw));
  // printf("mmu translate to paddr %p\n", (void *)(long)paddr);
  // } // printf("vaddr is 0x%x, paddr is 0x%x\n", vaddr, paddr);
  assert(vaddr >= 0x40000000 && vaddr <= 0xa1200000);
  // assert(paddr == vaddr);
  // return MEM_RET_FAIL;
  return paddr;
}
