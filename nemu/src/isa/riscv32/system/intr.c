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

#include <isa.h>
#define IRQ_TIMER 0x80000007
#define EVENT_IRQ_TIMER 5
/*#define get_mstatus_field(offset)      ((*s0 >> offset) & 0x1)
#define set_mstatus_field(offset, val) ((val == 0) ? (*s0 &= (~(1 << offset))) : (*s0 != (1 << offset)))
#define get_mstatus_mie()              get_mstatus_field(3)
#define set_mstatus_mie(val)           set_mstatus_field(3,val)
#define get_mstatus_mpie               get_mstatus_field(7)
#define set_mstatus_mpie(val)         set_mstatus_field(7, val)*/

#define MSTATUS_MIE 0x00000008
#define MSTATUS_MPIE 0x00000080
word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */
 // if(NO == -1 || NO == 0){
   // epc += 4;
  //}
  cpu.csr.mcause = NO;
  cpu.csr.mepc = epc;
  if(cpu.csr.mstatus & MSTATUS_MIE){
    cpu.csr.mstatus |= MSTATUS_MPIE;
  }else{
    cpu.csr.mstatus &= (~MSTATUS_MPIE);
  }
  cpu.csr.mstatus &= (~MSTATUS_MIE);
#ifdef CONFIG_ETRACE
  printf("event ID = %d,current PC = 0x%02x\n",NO,epc);
#endif
  return cpu.csr.mtvec;
}


word_t isa_query_intr() {
  if(cpu.INTR && (cpu.csr.mstatus & MSTATUS_MIE)){
    cpu.INTR = false;
    return IRQ_TIMER;
  }
  return INTR_EMPTY;
}
