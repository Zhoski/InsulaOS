#include <slib/stdio.h>
#include <slib/memory.h>
#include <slib/sys.h>
#include <slib/types.h>

void died() {
  asm("int $0x40"::"a"(0));
}

u32 main() {
  printf("TESTING DISK\n");
  for(u32 i = 0; i < 10;i++) {
    u32 fd = sys_open("/t.s");
    if(!fd) {
      printf("File open error\n");
      died();
    }

    sys_write(fd, "test", 0, 5);

    sys_read(fd, 0, 0x1000, (u8*)0x2000);
    printf("%s", (u8*)0x2000);
    
    sys_close(fd);
  }

  printf("TEST RUNNING AND EXIT\n");
  for(u32 i = 0; i < 10; i++) {
    sys_run("/apps/tasm.ndf",0,0);
  }
  
  died();
}
