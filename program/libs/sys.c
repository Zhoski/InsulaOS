#include <slib/sys.h>

unsigned int sys_read(unsigned int fd, unsigned int offset, unsigned int n, unsigned char* dest) {
    unsigned int ret = 0;
    asm volatile(
        "int $0x40"
        : "=a" (ret)
        : "a" (19), "b" (fd), "c" (offset), "d" (n), "S" (dest)
        : "memory"
    );
    return ret;
}

unsigned int sys_check(const char* f) {
    unsigned int ret = 0;
    asm volatile(
        "int $0x40"
        : "=a" (ret)
        : "a" (5), "b" (5), "S" (f)
        : "memory"
    );
    return ret;
}

unsigned int sys_write(unsigned int fd, unsigned char* src, unsigned int offset, unsigned int n) {
    unsigned int ret;
    asm volatile(
        "int $0x40"
        : "=a" (ret)
        : "a" (15), "b" (fd), "c" (offset), "d" (n), "S" (src)
        : "memory"
    );
    return ret;
}

unsigned int sys_create(const char* f, unsigned char flag) {
    unsigned int ret = 0;
    asm volatile(
        "int $0x40"
        : "=a" (ret)
        : "a" (7), "b" (f), "c"(flag)
        : "memory"
    );
    return ret;
}

unsigned int sys_delete(const char* node) {
    int ret = 0;
    asm volatile(
        "int $0x40"
        : "=a" (ret)
        : "a" (8), "b" (node)
        : "memory"
    );

    return ret;
}

unsigned int write_root(const char* dir) {
    unsigned int ret = 0;
    asm volatile(
        "int $0x40"
        : "=a" (ret)
        : "a" (16), "b" (dir)
        : "memory"
    );

    return ret;
}

unsigned int sys_run(const char* f, unsigned int argc, unsigned char* argv[]) {
    int ret = 0;
    asm volatile(
        "int $0x40"
        : "=a" (ret)
        : "a" (9), "b" (f), "c" (argc), "d" (argv)
        : "memory"
    );

    return ret;
}

unsigned int sys_died() {
    asm volatile(
        "int $0x40"
        :
        : "a" (0)
        : "memory"
    );
}

/**
 *  Возвращает дексриптор файла
 */
unsigned int sys_open(const char* f) {
    unsigned int ret;
    asm volatile(
        "int $0x40"
        : "=a" (ret)
        : "a" (5), "b" (f)
        : "memory"
    );
    
    return ret;
}

void sys_close(unsigned int fd) {
    asm volatile(
        "int $0x40"
        :
        : "a" (6), "b" (fd)
        : "memory"
    );
}
