all:
	riscv64-unknown-linux-gnu-as -march=rv32i add.s -o add.o
	riscv64-unknown-linux-gnu-ld add.o -o add.bin -m elf32lriscv -nostdlib --no-relax
	riscv64-unknown-linux-gnu-objcopy -O binary add.bin
	hexdump -e '"%08x\n"' add.bin > add.hex

clean:
	rm *.bin *.o
