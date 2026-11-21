#include <kernel/process.h>
#include <cpu/gdt.h>
#include <kernel/memory.h>
#include <fs/fs.h>
#include <drivers/video/video.h>
#include <lib/string.h>
#include <lib/other.h>
#include <drivers/timer/timer.h>
 
#define USR_RW_P   0x07
#define USR_RW     0x06
#define KRN_RW_P   0x03

#define FREE       0xFFFF

extern void jmp_usrmode(u32 entry, u32 stack);

Process prcs_tbl[MAX_PROCESS];
u32 cur_prcs = 0;
u32 prcs_start_executing = 0;

Fdesc f_desc_tbl[MAX_PROCESS];

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

void proc_swap() {
  if(prcs_start_executing)  {
    if(prcs_tbl[cur_prcs].state == FROZEN) return;
    prcs_tbl[cur_prcs].flag = 1;
    cur_prcs+=1;
    if(cur_prcs >= MAX_PROCESS) cur_prcs = 0;

    while(prcs_tbl[cur_prcs].pid == FREE && prcs_tbl[cur_prcs].state == ACTIVE) {
      cur_prcs++;
      if(cur_prcs >= MAX_PROCESS) cur_prcs = 0;
    }

    u32 i = 0;
    if(prcs_tbl[cur_prcs].flag == 1) i = 4;
    
    desc_t* tss_desc = (desc_t*)(gdt_addr + (prcs_tbl[cur_prcs].tss_sel << 3));
    tss_desc->acces &= ~0x02;
    ltr(prcs_tbl[cur_prcs].tss_sel);
    lldt(prcs_tbl[cur_prcs].ldt_sel);
   
    asm(
      "movl %0, %%esp\n"
      "movb $0x20, %%al\n"
      "outb %%al, $0x20\n"
      "outb %%al, $0xA0\n"
      "popl %%ds\n"
      "popl %%es\n"
      "popal\n"
      "sti\n"
      "iret"
      :
      : "r" (prcs_tbl[cur_prcs].kstack-i)
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

  prcs_tbl[cur_prcs].flag = 1;

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

u32 usr_md_get_proc(void* dest) {
  u32* proc_entry_size = dest;
  *proc_entry_size = sizeof(Process);
  memcpy(prcs_tbl, dest + 4, sizeof(Process) * MAX_PROCESS);
  return prcs_tbl[cur_prcs].sys_buffer;
}

void proc_kill(u32 pid) {
  desc_t* ldt_desc = gdt_addr + (prcs_tbl[pid].ldt_sel << 3);
  u32 base = ldt_desc->base_high << 24 | ldt_desc->base_mid << 16 | ldt_desc->base_low;
  gdt_descriptor_rm(prcs_tbl[pid].ldt_sel);
  gdt_descriptor_rm(prcs_tbl[pid].tss_sel);
  free((void*)prcs_tbl[pid].base);
  free((void*)prcs_tbl[pid].kstack);
  free((void*)base);
  free(prcs_tbl[pid].name);

  prcs_tbl[pid].pid = FREE;
  proc_swap();
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
  process.name = malloc(strlen(g_node.name) + 1);
  memcpy(g_node.name, process.name, strlen(g_node.name));
  process.heap_base = align(stack_limit + 1 + g_node.size + 0x1000, 1024, HIGHTER);
  process.stack_limit = stack_limit;
  process.sys_buffer = align(stack_limit, 1024, HIGHTER);
  process.limit = limit;
  process.base = base;
  process.state = ACTIVE;
  process.ldt_sel = ldt_mk_and_push(base, limit, stack_limit);
  process.kstack = (u32)malloc(1024) + 1020;//0x1B0000;//(u32)malloc(128) + 120;
  process.tss_sel = tss_mk_and_push(0x10, process.kstack);
  process.flag = 0;

  tbl_add_process(&process);
  return &prcs_tbl[process.pid];
}

u32 fdesc_desc_push(Fdesc fdesc) {
  for(u32 i = 0; i < MAX_PROCESS; i++) {
    if(f_desc_tbl[i].descriptor == 0) {
      memcpy(&fdesc, f_desc_tbl + i, sizeof(Fdesc));
      return i;
    }
  }
}

void fdesc_close(u32 descriptor) { 
  for(u32 i = 0; i < MAX_PROCESS; i++) {
    if(f_desc_tbl[i].descriptor == descriptor) {
      f_desc_tbl[i].descriptor = 0;
      free(f_desc_tbl[i].file);
      break;
    }
  }
}

u32 fdesc_open(u8* f, u32 pid) {
  if(!fs->check(f)) return 0;
  for(u32 i = 0; i < MAX_PROCESS; i++) {
    if(strcmp(f, f_desc_tbl[i].file) == 0) return 1;
  }
  
  Fdesc fdesc;
  fdesc.descriptor = tick;
  fdesc.owner = pid;

  fdesc.file = malloc(strlen(f) + 1);
  memcpy(f, fdesc.file, strlen(f));

  prcs_tbl[pid].fdesc = fdesc_desc_push(fdesc);

  return fdesc.descriptor;
}
