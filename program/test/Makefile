TARGET = ../../bin/test.ndf
CONV = ../../utilities/ndfconv/ndfconv

$(TARGET): test.elf
	$(CONV) test.elf $(TARGET)

test.elf : entry.o test.o stdio.o string.o sys.o memory.o
	gcc -m32 -nostdlib -static-pie \
	    -T ../general/user.ld entry.o stdio.o string.o sys.o memory.o test.o -o test.elf

test.o : main.c
	gcc -m32 -ffreestanding -fPIE -static-pie -fno-plt -Werror -Os -I./../libs/include -c main.c -o test.o

entry.o: ../general/entry.asm
	nasm -f elf32 ../general/entry.asm -o entry.o

stdio.o: ../libs/stdio.c
	gcc -m32 -ffreestanding -fPIE -static-pie -fno-plt -Werror -Os -I./../libs/include -c ../libs/stdio.c -o stdio.o

string.o: ../libs/string.c
	gcc -m32 -ffreestanding -fPIE -static-pie -fno-plt -Werror -Os -I./../libs/include -c ../libs/string.c -o string.o

sys.o: ../libs/sys.c
	gcc -m32 -ffreestanding -fPIE -static-pie -fno-plt -Werror -Os -I./../libs/include -c ../libs/sys.c -o sys.o

memory.o: ../libs/memory.c
	gcc -m32 -ffreestanding -fPIE -static-pie -fno-plt -Werror  -Os -I./../libs/include -c ../libs/memory.c -o memory.o

clean :
	rm *.o *.bin shell.elf
