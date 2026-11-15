#include <kernel/process.h>
#include <cpu/gdt.h>
#include <kernel/memory.h>
#include <fs/fs.h>
#include <drivers/video/video.h>
#include <lib/string.h>
#include <lib/align.h>

#define USR_RW_P   0x07
#define USR_RW     0x06
#define KRN_RW_P   0x03

#define FREE       0xFFFF

extern void jmp_usrmode(u32 entry, u32 stack);

Process prcs_tbl[MAX_PROCESS];
u32 cur_prcs = 0;
u32 prcs_start_executing = 0;

void prcs_tbl_init() {
  for(u32 i = 0; i < MAX_PROCESS; i++) {
    prcs_tbl[i].pid = FREE;
  }
}

void tbl_add_process(Process* process) {
  for(u32 i = 0; i < MAX_PROCESS; i++) {
    if(prcs_tbl[i].pid == FREE) {
      process->pid = i;
      memcpy(process, prcs_tbl + i, sizeof(Process));
      
      break;
    }
  }
}

void tbl_rm_process(u32 pid) {
  prcs_tbl[pid].pid = FREE;
}

void proc_swap() {
  if(prcs_start_executing) {
    cur_prcs=0;
    /*cur_prcs+=1;
    if(cur_prcs >= MAX_PROCESS) cur_prcs = 0;
    while(prcs_tbl[cur_prcs].pid == FREE &&
        prcs_tbl[cur_prcs].state != ACTIVE) {

        cur_prcs++;
        if(cur_prcs >= MAX_PROCESS) cur_prcs = 0;
    }*/

    ltr(prcs_tbl[cur_prcs].tss_sel);
    lldt(prcs_tbl[cur_prcs].ldt_sel);

    asm(
        "movl %0, %%esp\n"
        //"popl %%ds\n"
        //"popl %%es\n"
        //"popal\n"
        //"iret"
        :
        : "r" (prcs_tbl[cur_prcs].kstack)
        : "memory"
    );
  }
}

void proc_start_execute() {
  prcs_start_executing = 1;
  while(prcs_tbl[cur_prcs].pid == FREE &&
        prcs_tbl[cur_prcs].state != ACTIVE) {

    cur_prcs++;
    if(cur_prcs >= MAX_PROCESS) cur_prcs = 0;
  }

  ltr(prcs_tbl[cur_prcs].tss_sel);
  lldt(prcs_tbl[cur_prcs].ldt_sel);

  asm(
      "movl %0, %%esp\n"
      "popl %%ds\n"
      "popl %%es\n"
      "popal\n"
      "iret"
      :
      : "r" (prcs_tbl[cur_prcs].kstack)
      : "memory"
  );
}

/*
 * Возвращает указатель на процесс в prcs_tbl в случаи успеха и 0 в ином
*/
Process* process_spawn(u8* f) {
  u32 st = fs->check(f);
  if(!st) {
    return 0;
  }
  
  Process process;
  u32 limit = (64 << 10)-1;
  u32 base = (u32)malloc(limit);
  u32 stack_limit = (8 << 10)-1;
  process.heap_base = align(stack_limit + 1 + g_node.size, 1024, HIGHTER);
  process.stack_limit = stack_limit;
  process.limit = limit;
  process.base = base;
  process.state = ACTIVE;
  process.ldt_sel = ldt_mk_and_push(base, limit, stack_limit);
  process.kstack = (u32)malloc(1024) + 1020;//0x1B0000;//(u32)malloc(128) + 120;
  process.tss_sel = tss_mk_and_push(0x10, process.kstack);

  video->kprintf("S: %x\n", process.kstack);

  //for(;;);

  video->kprintf("LDT: %x\n", process.ldt_sel);
  video->kprintf("TSS: %x\n", process.tss_sel);

  //for(;;);

  tbl_add_process(&process);
  return &prcs_tbl[process.pid];
}
