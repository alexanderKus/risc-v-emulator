#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <stdexcept>
#include <format>

void inline log_error(const std::string err, const uint32_t inst) {
  std::cerr << "[ERROR] " << err << " 0x" << std::hex << inst << std::endl;
}

void inline log_debug_hex(const std::string name, const uint32_t x) {
#ifdef DEBUG
    std::cout << "[DEBUG] " << name << ": 0x" << std::hex << x << std::endl;
#endif
}

void inline log_info(const std::string data) {
#ifdef INFO
  std::cout << "[INFO] " << data << std::endl;
#endif
}

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
  FUNCT3_JALR = 0b00000000000000000000000000000000,

  // BRANCH
  FUNCT3_BEQ  = 0b00000000000000000000000000000000,
  FUNCT3_BNE  = 0b00000000000000000001000000000000,
  FUNCT3_BLT  = 0b00000000000000000100000000000000,
  FUNCT3_BGE  = 0b00000000000000000101000000000000,
  FUNCT3_BLTU = 0b00000000000000000110000000000000,
  FUNCT3_BGEU = 0b00000000000000000111000000000000,

  //LOAD
  FUNCT3_LOAD_BYTE   = 0b00000000000000000000000000000000,
  FUNCT3_LOAD_HALF   = 0b00000000000000000000000000000000,
  FUNCT3_LOAD_WORD   = 0b00000000000000000000000000000000,
  FUNCT3_LOAD_BYTE_U = 0b00000000000000000000000000000000,
  FUNCT3_LOAD_HAFL_U = 0b00000000000000000000000000000000,

  // STORE
  FUNCT3_STORE_BYTE = 0b00000000000000000000000000000000,
  FUNCT3_STORE_HALF = 0b00000000000000000001000000000000,
  FUNCT3_STORE_WORD = 0b00000000000000000010000000000000,

  // INT_COMP_I
  FUNCT3_ADDI  = 0b00000000000000000000000000000000,
  FUNCT3_SLTI  = 0b00000000000000000010000000000000,
  FUNCT3_SLTIU = 0b00000000000000000011000000000000,
  FUNCT3_XORI  = 0b00000000000000000100000000000000,
  FUNCT3_ORI   = 0b00000000000000000110000000000000,
  FUNCT3_ANDI  = 0b00000000000000000111000000000000,
  FUNCT3_SLLI  = 0b00000000000000000001000000000000,
  // NOTE: they are both equl to 5
  //FUNCT3_SRLI  = 0b101,
  FUNCT3_SRAI  = 0b00000000000000000101000000000000,

  // INT_COMP
  FUNCT3_ADD  = 0b00000000000000000000000000000000,
  FUNCT3_SUB  = 0b00000000000000000000000000000000,
  FUNCT3_SLL  = 0b00000000000000000001000000000000,
  FUNCT3_SLT  = 0b00000000000000000010000000000000,
  FUNCT3_SLTU = 0b00000000000000000011000000000000,
  FUNCT3_XOR  = 0b00000000000000000100000000000000,
  FUNCT3_SRL  = 0b00000000000000000101000000000000,
  FUNCT3_SRA  = 0b00000000000000000101000000000000,
  FUNCT3_OR   = 0b00000000000000000110000000000000,
  FUNCT3_AND  = 0b00000000000000000111000000000000,

  // FENCE
  FUNCT3_FENCE  = 0b00000000000000000000000000000000,
  FUNCT3_FENCEI = 0b00000000000000000001000000000000,

  // R
  FUNCT3_SCALL      = 0b00000000000000000000000000000000,
  FUNCT3_SBREAK     = 0b00000000000100000000000000000000,
  FUNCT3_SRDCYCLE   = 0b11000000000000000000000000000000,
  FUNCT3_SRDCYCLEH  = 0b11001000000000000000000000000000,
  FUNCT3_RDTIME     = 0b11000000000100000000000000000000,
  FUNCT3_RDTIMEH    = 0b11001000000100000000000000000000,
  FUNCT3_RDINSTRET  = 0b11000000001000000000000000000000,
  FUNCT3_RDINSTRETH = 0b11001000001000000000000000000000,

  // R
  IMM_SCALL      = 0b00000000000000000000000000000000,
  IMM_SBREAK     = 0b00000000000100000000000000000000,
  IMM_SRDCYCLE   = 0b11000000000000000000000000000000,
  IMM_SRDCYCLEH  = 0b11001000000000000000000000000000,
  IMM_RDTIME     = 0b11000000000100000000000000000000,
  IMM_RDTIMEH    = 0b11001000000100000000000000000000,
  IMM_RDINSTRET  = 0b11000000001000000000000000000000,
  IMM_RDINSTRETH = 0b11001000001000000000000000000000,
};

class Registers {
private:
  uint32_t _pc = 0;
  uint32_t _regs[32];
public:
  uint32_t get_pc() const {
    return this->_pc;
  }

  void increment_pc() {
    this->_pc++;
  }

  uint32_t& operator[](uint8_t index) {
    if (index < 0 || index >= 32) {
      throw std::out_of_range("Invalid register number");
      exit(1);
    }
    return this->_regs[index];
  }

};

class Ram {
private:
  uint32_t _data[1024 * 1024 * 4];
public:
  // OR instruction
  Ram() { this->_data[0] = 0b000000001100011110011100110011; }

  uint32_t read(uint32_t addr) const {
    return this->_data[addr];
  }

  void write(uint32_t addr, uint32_t data) {
    this->_data[addr] = data;
  }
};

class Instruction {
private:
  uint32_t _value;
public:
  Instruction(uint32_t value) {
    this->_value = value;
  }

  uint32_t get_value() const {
    return this->_value;
  }

  uint32_t get_opcode() const {
    return this->_value & 0b00000000000000000000000001111111;
  }
  
  uint32_t get_rd() const {
    return this->_value & 0b00000000000000000000111110000000;
  }

  uint32_t get_funct3() const {
    return this->_value & 0b00000000000000000111000000000000;
  }

  uint32_t get_rs1() const {
    return this->_value & 0b00000000000011111000000000000000;
  }

  uint32_t get_rs2() const {
    return this->_value & 0b00000001111100000000000000000000;
  }

  uint32_t get_funct7() const {
    return this->_value & 0b11111110000000000000000000000000;
  }

  uint32_t get_imm31_12() const {
    return this->_value & 0b11111111111111111111000000000000;
  }

  uint32_t get_imm11_0() const {
    return this->_value & 0b11111111111100000000000000000000;
  }
  
  uint32_t get_pred() const {
    return this->_value & 0b00001111000000000000000000000000;
  }

  uint32_t get_succ() const {
    return this->_value & 0b00000000111100000000000000000000;
  }
};

class RV32I {
private:
  Registers _regs;
  Ram _ram;

  Instruction* instruction_fetch() {
    uint32_t value = _ram.read(_regs.get_pc());
    _regs.increment_pc();
    Instruction* inst = new Instruction(value);
    return inst;
  }

  void instruction_decode(Instruction &inst) {
    uint32_t opcode = inst.get_opcode();
    log_debug_hex("opcode", opcode);
    switch(opcode) {
      case OPCODE_LUI: {
        log_info("opcode lui");
        uint32_t rd = inst.get_rd();
        uint32_t imm = inst.get_imm31_12();
        log_debug_hex("rd", rd);
        log_debug_hex("imm", imm);
        break;
      }
      case OPCODE_AUIPC: {
        log_info("opcode auipc");
        uint32_t rd = inst.get_rd();
        uint32_t imm = inst.get_imm31_12();
        log_debug_hex("rd", rd);
        log_debug_hex("imm", imm);
        break;
      }
      case OPCODE_JAL: {
        log_info("opcode jal");
        // TOOD: handle this command.
        uint32_t rd = inst.get_rd();
        uint32_t imm = inst.get_imm31_12();
        log_debug_hex("rd", rd);
        log_debug_hex("imm", imm);
        break;
      }
      case OPCODE_JALR: {
        log_info("opcode jalr");
        uint32_t rd = inst.get_rd();
        uint32_t funct3 = inst.get_funct3();
        uint32_t rs1 = inst.get_rs1();
        uint32_t imm = inst.get_imm11_0();
        log_debug_hex("rd", rd);
        log_debug_hex("funct3", funct3);
        log_debug_hex("rs1", rs1);
        log_debug_hex("imm", imm);
        break;
      }
      case OPCODE_BRANCH: {
        log_info("opcode branch");
        // TOOD: handle this command.
        uint32_t rd = inst.get_rd();
        uint32_t funct3 = inst.get_funct3();
        uint32_t rs1 = inst.get_rs1();
        uint32_t rs2 = inst.get_rs2();
        uint32_t imm = inst.get_imm11_0();
        log_debug_hex("imm[4:1|11]", rd);
        log_debug_hex("funct3", funct3);
        log_debug_hex("rs1", rs1);
        log_debug_hex("rs2", rs2);
        log_debug_hex("imm[12|10:5]", imm);
        break;
      }
      case OPCODE_LOAD: {
        log_info("opcode load");
        uint32_t rd = inst.get_rd();
        uint32_t funct3 = inst.get_funct3();
        uint32_t rs1 = inst.get_rs1();
        uint32_t imm = inst.get_imm11_0();
        log_debug_hex("rd", rd);
        log_debug_hex("funct3", funct3);
        log_debug_hex("rs1", rs1);
        log_debug_hex("imm", imm);
        break;
      }
      case OPCODE_STORE: {
        log_info("opcode store");
        // TOOD: handle this command.
        uint32_t rd = inst.get_rd();
        uint32_t funct3 = inst.get_funct3();
        uint32_t rs1 = inst.get_rs1();
        uint32_t rs2 = inst.get_rs2();
        uint32_t imm = inst.get_imm11_0();
        log_debug_hex("rd", rd);
        log_debug_hex("funct3", funct3);
        log_debug_hex("rs1", rs1);
        log_debug_hex("rs2", rs2);
        log_debug_hex("imm", imm);
        break;
      }
      case OPCODE_INT_COMP_I: {
        log_info("opcode int comp I");
        uint32_t rd = inst.get_rd();
        uint32_t funct3 = inst.get_funct3();
        uint32_t rs1 = inst.get_rs1();
        log_debug_hex("rd", rd);
        log_debug_hex("funct3", funct3);
        log_debug_hex("rs1", rs1);
        switch(funct3) {
          case FUNCT3_ADDI: {
            log_info("ADDI");
            uint32_t imm = inst.get_imm11_0();
            break;
          }
          case FUNCT3_SLTI: {
            log_info("SLTI");
            uint32_t imm = inst.get_imm11_0();
            break;
          }
          case FUNCT3_SLTIU: {
            log_info("SLTIU");
            uint32_t imm = inst.get_imm11_0();
            break;
          }
          case FUNCT3_XORI: {
            log_info("XORI");
            uint32_t imm = inst.get_imm11_0();
            break;
          }
          case FUNCT3_ORI: {
            log_info("ORI");
            uint32_t imm = inst.get_imm11_0();
            break;
          }
          case FUNCT3_ANDI: {
            log_info("ANDI");
            uint32_t imm = inst.get_imm11_0();
            break;
          }
          case FUNCT3_SLLI: {
            log_info("SLLI");
            uint32_t imm = inst.get_imm11_0();
            break;
          }
          // NOTE: they are both equl to 5
          //case FUNCT3_SRLI:
          case FUNCT3_SRAI: {
            uint32_t funct7 = inst.get_funct7();
            uint32_t shamt = inst.get_rs2();
            log_debug_hex("funct7", funct7);
            log_debug_hex("shamt", shamt);
            switch(funct7) {
              case 0x0: { // SRLI
                log_info("SRLI");
                break;
              }
              case 0x20000000: { // SRAI
                log_info("SRAI");
                break;
              }
              default: {
                log_error("Cannout decode instruction: 0x", inst.get_value());
                exit(1);
              }
            }
            break;
          }
        }
        break;
      }
      case OPCODE_INT_COMP_R: {
        log_info("opcode int comp R");
        uint32_t rd = inst.get_rd();
        uint32_t funct3 = inst.get_funct3();
        uint32_t rs1 = inst.get_rs1();
        uint32_t rs2 = inst.get_rs2();
        uint32_t funct7 = inst.get_funct7();
        log_debug_hex("rd", rd);
        log_debug_hex("funct3", funct3);
        log_debug_hex("rs1", rs1);
        log_debug_hex("rs2", rs2);
        log_debug_hex("funct7", funct7);
        switch(funct7) {
          case 0x0: {
            switch(funct3) {
              case FUNCT3_ADD: {
                log_info("ADD");
                break;
              }
              case FUNCT3_SLL: {
                log_info("SLL");
                break;
              }
              case FUNCT3_SLT: {
                log_info("SLT");
                break;
              }
              case FUNCT3_SLTU: {
                log_info("SLTU");
                break;
              }
              case FUNCT3_XOR: {
                log_info("XOR");
                break;
              }
              case FUNCT3_SRL: {
                log_info("SRL");
                break;
              }
              case FUNCT3_OR: {
                log_info("OR");
                break;
              }
              case FUNCT3_AND: {
                log_info("AND");
                break;
              }
              default: {
                log_error("Cannout decode instruction: 0x", inst.get_value());
                exit(1);
              }
            }
            break;
          }
          case 0x20000000: {
            switch(funct3) {
              case FUNCT3_SUB: {
                log_info("SUB");
                break;
              }
              case FUNCT3_SRA: {
                log_info("SRA");
                break;
              }
              default: {
                log_error("Cannout decode instruction: 0x", inst.get_value());
                exit(1);
              }
            }
            break;
          }
          default: {
            log_error("Cannout decode instruction: 0x", inst.get_value());
            exit(1);
          }
        }

        break;
      }
      case OPCODE_FENCE: {
        log_info("opcode fence");
        uint32_t rd = inst.get_rd();
        uint32_t funct3 = inst.get_funct3();
        uint32_t rs1 = inst.get_rs1();
        log_debug_hex("rd", rd);
        log_debug_hex("funct3", funct3);
        log_debug_hex("rs1", rs1);
        if (rs1 != 0x0) {
          log_error("Cannout decode instruction: 0x", inst.get_value());
          exit(1);
        }
        switch(funct3) {
          case FUNCT3_FENCE: {
            uint32_t pred = inst.get_pred();
            uint32_t succ = inst.get_succ();
            log_debug_hex("pred", pred);
            log_debug_hex("succ", succ);
            break;
          }
          case FUNCT3_FENCEI: {
            uint32_t imm = inst.get_imm11_0();
            log_debug_hex("imm", imm);
            if (imm != 0x0) {
              log_error("Cannout decode instruction", inst.get_value());
              exit(1);
            }
            break;
          }
          default: {
            log_error("Cannout decode instruction", inst.get_value());
            exit(1);
          }
        }
        break;
      }
      case OPCODE_R: {
        log_info("opcode R");
        uint32_t rd = inst.get_rd();
        uint32_t funct3 = inst.get_funct3();
        uint32_t rs1 = inst.get_rs1();
        log_debug_hex("rd", rd);
        log_debug_hex("funct3", funct3);
        log_debug_hex("rs1", rs1);
        if (rs1 != 0x0) {
          log_error("Cannout decode instruction: 0x", inst.get_value());
          exit(1);
        }
        if (rd == 0x0) {
          uint32_t imm = inst.get_imm11_0();
          log_debug_hex("imm", imm);
          switch(imm){
            case IMM_SCALL: {
              log_info("SCALL");
              break;
            }
            case IMM_SBREAK: {
              log_info("SBREAK");
              break;
            }
            case IMM_SRDCYCLE: {
              log_info("SRDCYCLE");
              break;
            }
            case IMM_SRDCYCLEH: {
              log_info("SRDCYCLEH");
              break;
            }
            case IMM_RDTIME: {
              log_info("RDTIME");
              break;
            }
            case IMM_RDTIMEH: {
              log_info("RDTIMEH");
              break;
            }
            case IMM_RDINSTRET: {
              log_info("RDINSTRET");
              break;
            }
            case IMM_RDINSTRETH: {
              log_info("RDINSTRETH");
              break;
            }
            default: {
              log_error("Cannout decode instruction", inst.get_value());
              exit(1);
            }
          }
        }
        break;
      }
      default: {
        log_error("Cannout decode instruction", inst.get_value());
        exit(1);
      }
    }
  }

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
    int i = 0;
    while (i++ < 1) {
      Instruction* inst = instruction_fetch();
      instruction_decode(*inst);
      execute();
      memory_access();
      write_back();
    }
  }
};



int main() {
  auto rv = new RV32I();
  rv->run();
  return 0;
}
