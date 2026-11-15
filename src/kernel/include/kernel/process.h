#ifndef PROCESS_H
#define PROCESS_H

#include <lib/int.h>

#define MAX_PROCESS    32

typedef enum {
  ACTIVE,
  SLEEP,
  DIED,
} State;

typedef struct {
  u32 pid;
  u32 ldt_sel;
  u32 tss_sel;
  u32 base;
  u32 limit;
  u32 stack_limit;
  u32 kstack;
  u32 heap_base;
  u32 flag;
  State state;
} Process;


void prcs_tbl_init();
void proc_start_execute();
void proc_swap();
Process* process_spawn(u8* f);

extern Process prcs_tbl[MAX_PROCESS];
extern u32 cur_prcs;
extern u32 prcs_start_executing;

#endif
