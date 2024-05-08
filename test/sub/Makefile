all:
	riscv64-unknown-linux-gnu-as -march=rv32i sub.s -o sub.o
	riscv64-unknown-linux-gnu-ld sub.o -o sub.bin -m elf32lriscv -nostdlib --no-relax
	riscv64-unknown-linux-gnu-objcopy -O binary sub.bin
	hexdump -e '"%08x\n"' sub.bin > sub.hex

clean:
	rm *.bin *.o
