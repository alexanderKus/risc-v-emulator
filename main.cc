#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <stdexcept>
#include <format>


class Registers {
public:
  uint32_t x0 = 0;  // zero
  uint32_t x1 = 0;  // ra - return addres
  uint32_t x2 = 0;  // sp - stack pointer
  uint32_t x3 = 0;  // gp - global pointer
  uint32_t x4 = 0;  // tp - thread pointer
  uint32_t x5 = 0;  // t0 - temporary
  uint32_t x6 = 0;  // t1 - temporary
  uint32_t x7 = 0;  // t2 - temporary
  uint32_t x8 = 0;  // fp/s0 - save register / frame pointer
  uint32_t x9 = 0;  // s1 - save register
  uint32_t x10 = 0; // a0 - function argument / return value
  uint32_t x11 = 0; // a1 - function argument / return value
  uint32_t x12 = 0; // a2 - function arugment  
  uint32_t x13 = 0; // a3
  uint32_t x14 = 0; // a4
  uint32_t x15 = 0; // a5
  uint32_t x16 = 0; // a6
  uint32_t x17 = 0; // a7
  uint32_t x18 = 0; // s2 - saved register
  uint32_t x19 = 0; // s3
  uint32_t x20 = 0; // s4
  uint32_t x21 = 0; // s5
  uint32_t x22 = 0; // s6
  uint32_t x23 = 0; // s7
  uint32_t x24 = 0; // s8
  uint32_t x25 = 0; // s9
  uint32_t x26 = 0; // s10
  uint32_t x27 = 0; // s11
  uint32_t x28 = 0; // t3 - temporary
  uint32_t x29 = 0; // t4
  uint32_t x30 = 0; // t5
  uint32_t x31 = 0; // t6
  uint32_t pc = 0;
};

enum {
  OPCODE_LUI        = 0b00110111,
  OPCODE_AUIPC      = 0b00010111,
  OPCODE_JAL        = 0b01101111,
  OPCODE_JALR       = 0b01100111,
  OPCODE_BRANCH     = 0b01100011,
  OPCODE_LOAD       = 0b00000011,
  OPCODE_STORE      = 0b01000011,
  OPCODE_INT_COMP_I = 0b00100011,
  OPCODE_INT_COMP_R = 0b00110011,
  OPCODE_FENCE      = 0b00001111,
  // NOTE: figure out better name than R
  OPCODE_R          = 0b01110011,
};

enum {
  FUNC3_JALR = 0b000,

  // BRANCH
  FUNC3_BEQ  = 0b000,
  FUNC3_BNE  = 0b001,
  FUNC3_BLT  = 0b100,
  FUNC3_BGE  = 0b101,
  FUNC3_BLTU = 0b110,
  FUNC3_BGEU = 0b111,

  //LOAD
  FUNC3_LOAD_BYTE   = 0b000,
  FUNC3_LOAD_HALF   = 0b000,
  FUNC3_LOAD_WORD   = 0b000,
  FUNC3_LOAD_BYTE_U = 0b000,
  FUNC3_LOAD_HAFL_U = 0b000,

  // STORE
  FUNC3_STORE_BYTE = 0b000,
  FUNC3_STORE_HALF = 0b001,
  FUNC3_STORE_WORD = 0b010,

  // INT_COMP_I
  FUNC3_ADDI  = 0b000,
  FUNC3_SLTI  = 0b010,
  FUNC3_SLTIU = 0b011,
  FUNC3_XORI  = 0b100,
  FUNC3_ORI   = 0b110,
  FUNC3_ANDI  = 0b111,
  FUNC3_SLLI  = 0b001,
  FUNC3_SRLI  = 0b101,
  FUNC3_SRAI  = 0b101,

  // INT_COMP
  FUNC3_ADD  = 0b000,
  FUNC3_SUB  = 0b000,
  FUNC3_SLL  = 0b001,
  FUNC3_SLT  = 0b010,
  FUNC3_SLTU = 0b011,
  FUNC3_XOR  = 0b100,
  FUNC3_SRL  = 0b101,
  FUNC3_SRA  = 0b101,
  FUNC3_OR   = 0b110,
  FUNC3_AND  = 0b111,

  // FENCE
  FUNC3_FENCE  = 0b000,
  FUNC3_FENCEI = 0b001,

  // R
  FUNC3_SCALL      = 0b000,
  FUNC3_SBREAK     = 0b000,
  FUNC3_SRDCYCLE   = 0b000,
  FUNC3_SRDCYCLEH  = 0b000,
  FUNC3_RDTIME     = 0b000,
  FUNC3_RDTIMEH    = 0b000,
  FUNC3_RDINSTRET  = 0b000,
  FUNC3_RDINSTRETH = 0b000,

  // R
  FUNC7_SCALL      = 0b000000000000,
  FUNC7_SBREAK     = 0b000000000001,
  FUNC7_SRDCYCLE   = 0b110000000000,
  FUNC7_SRDCYCLEH  = 0b110010000000,
  FUNC7_RDTIME     = 0b110000000001,
  FUNC7_RDTIMEH    = 0b110010000001,
  FUNC7_RDINSTRET  = 0b110000000010,
  FUNC7_RDINSTRETH = 0b110010000010,
};


class Ram {
private:
  uint32_t _data[1024 * 1024 * 4];
public:
  Ram() { }

  uint32_t read(uint32_t addr) const {
    return _data[addr];
  }

  void write(uint32_t addr, uint32_t data) {
    _data[addr] = data;
  }
};

class Instruction {
private:
  uint32_t _value;
public:
  Instruction(uint32_t value) {
    this->_value = value;
  }

  uint32_t getValue() const {
    return this->_value;
  }

  uint32_t getOpcode() const {
    return this->_value & 0xfffffF82;
  }
};

class RV32I {
private:
  Registers _regs;


  void instruction_fetch() {}
  void instruction_decode() {}
  void execute() {}
  void memory_access() {}
  void write_back() {}

public:
  RV32I() { }

  /*
   * Pipeline
   * 1. IF (instruction fetch)
   * 2. ID (instruction decode)
   * 3. EX (execute)
   * 4. MA (memory access)
   * 5. WB (write back)
   */
  void run() {
     
  }
};



int main() {
  auto rv = new RV32I();

  return 0;
}
