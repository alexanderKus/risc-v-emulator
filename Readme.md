# RISC-V CPU emulator 

### Build

Just run `make` to build `main`.


For more verbose output use `make info` or `make debug`.

### Tests

Build test exmaple

```
cd test/add
make
```

Running test: `./main test/add/add.bin`

### RISC V Toolchain

Offical [repo](https://github.com/riscv-collab/riscv-gnu-toolchain).


To build assemby file run those commands:
```bash
riscv64-unknown-linux-gnu-as -march=rv32i add.s -o file.o
riscv64-unknown-linux-gnu-ld add.o -o file.bin -m elf32lriscv -nostdlib --no-relax
riscv64-unknown-linux-gnu-objcopy -O binary file.bin
```

### Instruction Set
 - [x] R32I

### TODO
 - [ ] Implement some ecalls functions
 - [ ] Write more test cases
