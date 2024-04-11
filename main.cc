#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <stdexcept>
#include <format>

/*
 *  Memory Map
 *  ROM: 0x10000000
 *  RAM: 0x20000000
 */

enum MemoryMap {
  ROM_ADDR = 0x10000000,
  RAM_ADDR = 0x20000000
};

// NOTE: rom and ram sizes are the same. 
#define ROM_SIZE 1024 * 1024
#define RAM_SIZE 1024 * 1024 * 4

class ROM {
private:
  uint32_t _mask = (ROM_SIZE / 4) - 1;
  std::vector<uint32_t> _rom;

public:
  ROM() {
    this->_rom.resize(ROM_SIZE / 4);
  }

  uint32_t read(uint32_t addr) const {
    const uint32_t a = addr >> 2; // division by 4
    auto value = this->_rom.at(a & _mask);
    std::cout << std::format("Read  ROM addr 0x{:x} value 0x{:x}", addr, value) << std::endl;
    return value;
  }

  void load(std::vector<uint32_t> &data) {
    for(uint32_t i = 0; i < ROM_SIZE / 4; i++) {
        if(i >= data.size()) this->_rom.at(i) = 0xffffffff; 
        else this->_rom.at(i) = data.at(i);
      }
  }
};

class RAM {
private:
  uint32_t _mask = (RAM_SIZE / 4) - 1;
  std::vector<uint32_t> _ram;

public:
  RAM() {
    this->_ram.resize(RAM_SIZE / 4);
  }

  uint32_t read(uint32_t addr) const {
    const uint32_t a = addr >> 2;
    auto value = this->_ram.at(a & _mask);
    std::cout << std::format("Read  RAM addr 0x{:x} value 0x{:x}", addr, value) << std::endl;
    return value;
  }

  void write(uint32_t addr, uint32_t value) {
    const uint32_t a = addr >> 2;
    std::cout << std::format("Write RAM addr 0x{:x} value 0x{:x}", addr, value) << std::endl;
    this->_ram.at(a & _mask) = value;
  }
};

// Interface for MMIO
class MMIOSystem {
public:
  MMIOSystem(ROM &rom, RAM &ram) {
    this->_rom = rom;
    this->_ram = ram;
  }

  uint32_t read(uint32_t addr) const {
    if ((addr & ROM_ADDR) == ROM_ADDR) {
      return this->_rom.read(addr & 0x0fffffff);
    }
    else if ((addr & RAM_ADDR) == RAM_ADDR) {
      return this->_ram.read(addr & 0x0fffffff);
    }

    throw std::invalid_argument("unhandled addr 0x:" + std::format("{:x}", addr));
    return 0;
  }

  void write(uint32_t addr, uint32_t value) {
    if ((addr & ROM_ADDR) == ROM_ADDR) {
      // cannot write to ROM!
    }
    else if ((addr & RAM_ADDR) == RAM_ADDR) {
      this->_ram.write(addr & 0x0fffffff, value);
    }
  }

  // NOTE: this should not be allowed!!!
  void load(std::vector<uint32_t> &data) {
    this->_rom.load(data);
  }

private:
  ROM _rom;
  RAM _ram;
};

// RISC-V 32bit base instruction set
class RV32I {
public:
  RV32I() {
  }

  ROM* rom = new ROM();
  RAM* ram = new RAM();
  MMIOSystem* bus = new MMIOSystem(*this->rom, *this->ram);
};



int main() {
  auto rv = new RV32I();

  auto a = std::vector<uint32_t>(2, 0x1337);
  rv->bus->load(a);
  rv->bus->read(0x10000000);
  rv->bus->read(0x10000004);

  rv->bus->write(0x20001000, 0x1337);
  rv->bus->read(0x20001000);

/*
 * 1024 * 1024 = 0x00400000 end of memory
 * Start writing data at the beginnig
 *
 * rv->bus->write(0x20000000, 0x1337);
 * rv->bus->read(0x20000000);
 * rv->bus->write(0x20400000, 0x2137);
 * rv->bus->read(0x20000000);
 */

  return 0;
}
