CC = gcc
CFLAGS = -m32 -ffreestanding -fno-pie -nostdlib -nostdinc -Iinclude -Wall -Wextra

OBJS = kernel/entry.o kernel/isr.o kernel/switch.o kernel/kernel.o kernel/idt.o kernel/pic.o kernel/keyboard.o kernel/shell.o kernel/util.o kernel/paging.o kernel/heap.o kernel/virtio.o kernel/virtio_net.o kernel/pci.o kernel/myfs.o kernel/task.o kernel/test_tasks.o kernel/mouse.o kernel/mouse_cursor.o kernel/gui.o kernel/games.o kernel/minesweeper.o kernel/game2048.o kernel/fileman.o kernel/calc.o kernel/dmesg.o kernel/gdt.o kernel/gdt_flush.o kernel/usermode.o kernel/usermode_asm.o kernel/syscall.o kernel/loader.o kernel/shell_loop.o

all: kernel.elf

kernel/entry.o: kernel/entry.asm
	nasm -f elf32 $< -o $@

kernel/isr.o: kernel/isr.asm
	nasm -f elf32 $< -o $@

kernel/switch.o: kernel/switch.asm
	nasm -f elf32 $< -o $@

kernel/gdt_flush.o: kernel/gdt_flush.asm
	nasm -f elf32 $< -o $@

kernel/usermode_asm.o: kernel/usermode.asm
	nasm -f elf32 $< -o $@

kernel/kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/idt.o: kernel/idt.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/pic.o: kernel/pic.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/keyboard.o: kernel/keyboard.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/shell.o: kernel/shell.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/util.o: kernel/util.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/paging.o: kernel/paging.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/heap.o: kernel/heap.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/virtio.o: kernel/virtio.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/virtio_net.o: kernel/virtio_net.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/pci.o: kernel/pci.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/myfs.o: kernel/myfs.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/task.o: kernel/task.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/test_tasks.o: kernel/test_tasks.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/mouse.o: kernel/mouse.c
	$(CC) $(CFLAGS) -c $< -o $@


kernel/dmesg.o: kernel/dmesg.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/gdt.o: kernel/gdt.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/usermode.o: kernel/usermode.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/syscall.o: kernel/syscall.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/loader.o: kernel/loader.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/shell_loop.o: kernel/shell_loop.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.elf: $(OBJS) linker.ld
	ld -m elf_i386 -T linker.ld -o $@ $(OBJS)

run: os.img
	qemu-system-i386 -m 1024 -drive file=os.img,format=raw,if=ide,index=0 -drive file=disk.img,format=raw,if=virtio -netdev user,id=net0 -device virtio-net-pci,netdev=net0 -boot c -rtc base=localtime

clean:
	rm -f kernel/*.o kernel.elf


# Образ для загрузки
os.img: boot/boot.bin kernel.bin
	dd if=/dev/zero of=os.img bs=512 count=20480 2>/dev/null
	dd if=boot/boot.bin of=os.img conv=notrunc 2>/dev/null
	dd if=kernel.bin of=os.img bs=512 seek=1 conv=notrunc 2>/dev/null

kernel.bin: kernel.elf
	objcopy -O binary $< $@

boot/boot.bin: boot/boot.asm
	nasm -f bin $< -o $@

run2: os.img
	qemu-system-i386 -m 1024 -drive file=os.img,format=raw,if=ide,index=0 -drive file=disk.img,format=raw,if=virtio -netdev user,id=net0 -device virtio-net-pci,netdev=net0 -boot c -rtc base=localtime
