#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <stdexcept>
#include <format>
#include <fstream>
#include <cstdint>
#include <algorithm>

void inline log_error(const std::string err, const uint32_t x) {
  std::cerr << "[ERROR] " << err << " 0x" << std::hex << x << std::endl;
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

constexpr int32_t sext(int32_t imm, int bits) {
  int sign_pos = bits - 1;
  if (imm & (1 << sign_pos)) {
    int32_t extended_imm = imm | (0xffffffff << bits);
  }
  return imm;
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
  FUNCT3_LOAD_HALF   = 0b00000000000000000001000000000000,
  FUNCT3_LOAD_WORD   = 0b00000000000000000010000000000000,
  FUNCT3_LOAD_BYTE_U = 0b00000000000000000100000000000000,
  FUNCT3_LOAD_HALF_U = 0b00000000000000000101000000000000,

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
  uint32_t _regs[32] = {0};
public:
  uint32_t get_pc() const {
    return this->_pc;
  }

  void set_pc(uint32_t val) {
    this->_pc = val;
  }

  void increment_pc() {
    this->_pc++;
  }

  void increment_pc_by_offset(uint32_t offset) {
    this->_pc += this->_pc + offset;
  }

  uint32_t& operator[](uint8_t index) {
    if (index < 0 || index >= 32) {
      throw std::out_of_range("Invalid register number");
      exit(1);
    }
    return this->_regs[index];
  }

  void dump_regs() const {
    std::cout << "\tDumping regs...\n";
    for(int i = 0; i < 32; i++) {
      std::cout << "\tx-" << i << ": 0x" << std::hex << _regs[i] << '\n';
    }
    std::cout << "\tDone" << std::endl;
  }

};

class Ram {
private:
  uint32_t _data[1024 * 1024 * 4];

  bool is_valid(uint32_t addr) const {
    return addr >= 0 || addr < (1024 * 1024 * 4);
  }
public:
  // OR instruction
  Ram() { /* this->_data[0] = 0b000000001100011110011100110011; */}

  uint32_t read(uint32_t addr) const {
    if (this->is_valid(addr)) {
      return this->_data[addr];
    }
    log_error("[READ] Invalid address", addr);
    exit(1);
  }

  void write(uint32_t addr, uint32_t data) {
    if (this->is_valid(addr)) { 
      this->_data[addr] = data;
      return;
    }
    log_error("[WRITE] Invalid address", addr);
    exit(1);
  }
  
  void load(std::vector<uint32_t> payload) {
    for(uint32_t i = 0; i < payload.size(); i++) {
      write(i, payload.at(i));
    }
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
    return (this->_value & 0b00000000000000000000111110000000) >> 7;
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
    return this->_value & 0b11111111111111111111000000000000 >> 12;
  }

  uint32_t get_imm11_0() const {
    return this->_value & 0b11111111111100000000000000000000 >> 20;
  }

  uint32_t get_imm_store() const {
    return ((this->_value & 0b11111111000000000000000000000000) >> 25) |
           ((this->_value & 0b00000000000000000000111110000000) >> 7);
  }

  uint32_t get_imm_branch() const {
    return ((this->_value & 0b10000000000000000000000000000000) >> 19) |
           ((this->_value & 0b01111110000000000000000000000000) >> 20) |
           (((this->_value & 0b00000000000000000000111100000000) >> 7) & 0x1E) | // NOTE: & 0x1E is becauce bit at 0 position is alawys 0.
           ((this->_value & 0b00000000000000000000000010000000) << 4);
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

  void execute(Instruction &inst) {
    uint32_t opcode = inst.get_opcode();
    log_debug_hex("opcode", opcode);
    switch(opcode) {
      case OPCODE_LUI: {
        log_info("opcode lui");
        uint32_t rd = inst.get_rd();
        uint32_t imm = inst.get_imm31_12();
        log_debug_hex("rd", rd);
        log_debug_hex("imm", imm);
        // TODO: verify if correct.
        _regs[rd] = sext(imm, 32);
        break;
      }
      case OPCODE_AUIPC: {
        log_info("opcode auipc");
        uint32_t rd = inst.get_rd();
        uint32_t imm = inst.get_imm31_12();
        log_debug_hex("rd", rd);
        log_debug_hex("imm", imm);
        // TODO: verify if correct.
        _regs[rd] = (uint32_t)sext(imm, 32) + _regs.get_pc();
        break;
      }
      case OPCODE_JAL: {
        log_info("opcode jal");
        uint32_t rd = inst.get_rd();
        uint32_t imm = inst.get_imm31_12();
        log_debug_hex("rd", rd);
        log_debug_hex("imm", imm);
        // TODO: verify if correct.
        _regs[rd] = _regs.get_pc() + 1; // NOTE: Originally 4 is added.
        uint32_t offset = (uint32_t)sext(imm, 20);
        log_info("JAL");
        _regs.increment_pc_by_offset(offset);
        break;
      }
      case OPCODE_JALR: {
        log_info("opcode jalr");
        uint32_t rd = inst.get_rd();
        uint32_t funct3 = inst.get_funct3();
        uint32_t rs1 = inst.get_rs1();
        uint32_t imm = inst.get_imm11_0();
        uint32_t jaddr = (_regs[rs1] + sext(imm, 12)) & ~1;
        log_debug_hex("rd", rd);
        log_debug_hex("funct3", funct3);
        log_debug_hex("rs1", rs1);
        log_debug_hex("imm", imm);
        log_debug_hex("jaddr", jaddr);
        // NOTE: +1 not +4
        log_info("JALR");
        uint32_t t = _regs.get_pc() + 1; 
        _regs.set_pc(jaddr);
        _regs[rd] = t;
        break;
      }
      case OPCODE_BRANCH: {
        log_info("opcode branch");
        uint32_t funct3 = inst.get_funct3();
        uint32_t rs1 = inst.get_rs1();
        uint32_t rs2 = inst.get_rs2();
        uint32_t imm = inst.get_imm_branch();
        log_debug_hex("funct3", funct3);
        log_debug_hex("rs1", rs1);
        log_debug_hex("rs2", rs2);
        log_debug_hex("imm_branch", imm);
        switch(funct3){
          case FUNCT3_BEQ: {
            log_info("BEQ - check");
            if (_regs[rs1] == _regs[rs2]) {
              log_info("BEQ");
              _regs.increment_pc_by_offset(sext(imm, 12));
            }
            break;
          }
          case FUNCT3_BNE: {
            log_info("BNE - check");
            if (_regs[rs1] != _regs[rs2]) {
              log_info("BNE");
              _regs.increment_pc_by_offset(sext(imm, 12));
            }
            break;
          }
          case FUNCT3_BLT: {
            log_info("BLT - check");
            if ((int32_t)_regs[rs1] < (int32_t)_regs[rs2]) {
              log_info("BLT");
              _regs.increment_pc_by_offset(sext(imm, 12));
            }
            break;
          }
          case FUNCT3_BGE: {
            log_info("BGE - check");
            if ((int32_t)_regs[rs1] >= (int32_t)_regs[rs2]) {
              log_info("BGE");
              _regs.increment_pc_by_offset(sext(imm, 12));
            }
            break;
          }
          case FUNCT3_BLTU: {
            log_info("BLTU - check");
            if (_regs[rs1] < _regs[rs2]) {
              log_info("BLTU");
              _regs.increment_pc_by_offset(sext(imm, 12));
            }
            break;
          }
          case FUNCT3_BGEU: {
            log_info("BGEU - check");
            if (_regs[rs1] >= _regs[rs2]) {
            log_info("BGEU");
              _regs.increment_pc_by_offset(sext(imm, 12));
            }
            break;
          }
          default: {
            log_error("[BRANCH] Cannot decode instruction: 0x", inst.get_value());
            exit(1);
          }
        }
        break;
      }
      case OPCODE_LOAD: {
        log_info("opcode load");
        uint32_t rd = inst.get_rd();
        uint32_t funct3 = inst.get_funct3();
        uint32_t rs1 = inst.get_rs1();
        uint32_t imm = inst.get_imm11_0();
        uint32_t addr = (uint32_t)((int32_t)_regs[rs1] + sext(imm, 12));
        log_debug_hex("rd", rd);
        log_debug_hex("funct3", funct3);
        log_debug_hex("rs1", rs1);
        log_debug_hex("imm", imm);
        log_debug_hex("addr", addr);
        // NOTE: I do not think there is a difference between signed and unsiged load.
        switch(funct3) {
          case FUNCT3_LOAD_BYTE: {
            log_info("LB");
            _regs[rd] = _ram.read(addr) & 0x000000FF;
            break;
          }
          case FUNCT3_LOAD_HALF: {
            log_info("LH");
            _regs[rd] = _ram.read(addr) & 0x0000FFFF;
            break;
          }
          case FUNCT3_LOAD_WORD: {
            log_info("LW");
            _regs[rd] = _ram.read(addr) & 0xFFFFFFFF;
            break;
          }
          case FUNCT3_LOAD_BYTE_U: {
            log_info("LBU");
            _regs[rd] = _ram.read(addr) & 0x000000FF;
            break;
          }
          case FUNCT3_LOAD_HALF_U: {
            log_info("LHU");
            _regs[rd] = _ram.read(addr) & 0x0000FFFF;
            break;
          }
          default: {
            log_error("[LOAD]Cannot decode instruction: 0x", inst.get_value());
            exit(1);
          }
        }
        break;
      }
      case OPCODE_STORE: {
        log_info("opcode store");
        uint32_t rd = inst.get_rd();
        uint32_t funct3 = inst.get_funct3();
        uint32_t rs1 = inst.get_rs1();
        uint32_t rs2 = inst.get_rs2();
        uint32_t imm = inst.get_imm_store();
        uint32_t addr = (uint32_t)((int32_t)_regs[rs1] + sext(imm, 12));
        log_debug_hex("rd", rd);
        log_debug_hex("funct3", funct3);
        log_debug_hex("rs1", rs1);
        log_debug_hex("rs2", rs2);
        log_debug_hex("imm", imm);
        log_debug_hex("addr", addr);
        switch(funct3) {
          case FUNCT3_STORE_BYTE: {
            uint32_t val = _regs[rs2] & 0b00000000000000000000000001111111;
            _ram.write(addr, val);
            break;
          }
          case FUNCT3_STORE_HALF: {
            uint32_t val = _regs[rs2] & 0b00000000000000001111111111111111;
            _ram.write(addr, val);
            break;
          }
          case FUNCT3_STORE_WORD: {
            uint32_t val = _regs[rs2];
            _ram.write(addr, val);
            break;
          }
          default: {
            log_error("[STORE] Cannot decode instruction: 0x", inst.get_value());
            exit(1);
          }
        }
        break;
      }
      case OPCODE_INT_COMP_I: {
        log_info("opcode int comp I");
        uint32_t rd = inst.get_rd();
        uint32_t funct3 = inst.get_funct3();
        uint32_t rs1 = inst.get_rs1();
        int32_t imm = inst.get_imm11_0();
        log_debug_hex("rd", rd);
        log_debug_hex("funct3", funct3);
        log_debug_hex("rs1", rs1);
        log_debug_hex("imm", imm);
        switch(funct3) {
          case FUNCT3_ADDI: {
            log_info("ADDI");
            // TODO: verify if this correct.
            _regs[rd] = _regs[rs1] + sext(imm, 12);
            break;
          }
          case FUNCT3_SLTI: {
            log_info("SLTI");
            _regs[rd] = (int32_t)_regs[rs1] < sext(imm, 12);
            break;
          }
          case FUNCT3_SLTIU: {
            log_info("SLTIU");
            _regs[rd] = _regs[rs1] < (uint32_t)sext(imm, 12);
            break;
          }
          case FUNCT3_XORI: {
            log_info("XORI");
            _regs[rd] = _regs[rs1] ^ sext(imm, 12);
            break;
          }
          case FUNCT3_ORI: {
            log_info("ORI");
            _regs[rd] = _regs[rs1] | sext(imm, 12);
            break;
          }
          case FUNCT3_ANDI: {
            log_info("ANDI");
            _regs[rd] = _regs[rs1] & sext(imm, 12);
            break;
          }
          case FUNCT3_SLLI: {
            log_info("SLLI");
            uint32_t funct7 = inst.get_funct7();
            uint32_t shamt = inst.get_rs2();
            _regs[rd] = _regs[rs1] << shamt;
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
              case 0x0: {
                log_info("SRLI");
                // TODO: Fix this right logical shift
                _regs[rd] = _regs[rs1] >> shamt;
                break;
              }
              case 0x20000000: {
                log_info("SRAI");
                _regs[rd] = _regs[rs1] >> shamt;
                break;
              }
              default: {
                log_error("[SRAI] Cannot decode instruction: 0x", inst.get_value());
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
                _regs[rd] = _regs[rs1] + _regs[rs2];
                break;
              }
              case FUNCT3_SLL: {
                log_info("SLL");
                _regs[rd] = _regs[rs1] << _regs[rs2];
                break;
              }
              case FUNCT3_SLT: {
                log_info("SLT");
                _regs[rd] = ((int32_t)_regs[rs1]) < ((int32_t)_regs[rs2]);
                break;
              }
              case FUNCT3_SLTU: {
                log_info("SLTU");
                _regs[rd] = _regs[rs1] < _regs[rs2];
                break;
              }
              case FUNCT3_XOR: {
                log_info("XOR");
                _regs[rd] = _regs[rs1] ^ _regs[rs2];
                break;
              }
              case FUNCT3_SRL: {
                log_info("SRL");
                // TODO: Fix this right logical shift
                _regs[rd] = _regs[rs1] >> _regs[rs2];
                break;
              }
              case FUNCT3_OR: {
                log_info("OR");
                _regs[rd] = _regs[rs1] | _regs[rs2];
                break;
              }
              case FUNCT3_AND: {
                log_info("AND");
                _regs[rd] = _regs[rs1] & _regs[rs2];
                break;
              }
              default: {
                log_error("[COMP_R1] Cannot decode instruction: 0x", inst.get_value());
                exit(1);
              }
            }
            break;
          }
          case 0x20000000: {
            switch(funct3) {
              case FUNCT3_SUB: {
                log_info("SUB");
                _regs[rd] = _regs[rs1] - _regs[rs2];
                break;
              }
              case FUNCT3_SRA: {
                log_info("SRA");
                _regs[rd] = _regs[rs1] >> _regs[rs2];
                break;
              }
              default: {
                log_error("[COMP_R2] Cannot decode instruction: 0x", inst.get_value());
                exit(1);
              }
            }
            break;
          }
          default: {
            log_error("[COMP_R] Cannot decode instruction: 0x", inst.get_value());
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
          log_error("[FENCE1] Cannot decode instruction: 0x", inst.get_value());
          exit(1);
        }
        switch(funct3) {
          case FUNCT3_FENCE: {
            uint32_t pred = inst.get_pred();
            uint32_t succ = inst.get_succ();
            log_debug_hex("pred", pred);
            log_debug_hex("succ", succ);
            throw std::runtime_error("Insctruion not implemented yet.");
            break;
          }
          case FUNCT3_FENCEI: {
            uint32_t imm = inst.get_imm11_0();
            log_debug_hex("imm", imm);
            if (imm != 0x0) {
              log_error("[FENCE2] Cannot decode instruction", inst.get_value());
              exit(1);
            }
            throw std::runtime_error("Insctruion not implemented yet.");
            break;
          }
          default: {
            log_error("[FENCE] Cannot decode instruction", inst.get_value());
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
          log_error("[R1] Cannot decode instruction: 0x", inst.get_value());
          exit(1);
        }
        if (rd == 0x0) {
          uint32_t imm = inst.get_imm11_0();
          log_debug_hex("imm", imm);
          switch(imm){
            case IMM_SCALL: {
              log_info("SCALL");
              throw std::runtime_error("Insctruion not implemented yet.");
              break;
            }
            case IMM_SBREAK: {
              log_info("SBREAK");
              throw std::runtime_error("Insctruion not implemented yet.");
              break;
            }
            case IMM_SRDCYCLE: {
              log_info("SRDCYCLE");
              throw std::runtime_error("Insctruion not implemented yet.");
              break;
            }
            case IMM_SRDCYCLEH: {
              log_info("SRDCYCLEH");
              throw std::runtime_error("Insctruion not implemented yet.");
              break;
            }
            case IMM_RDTIME: {
              log_info("RDTIME");
              throw std::runtime_error("Insctruion not implemented yet.");
              break;
            }
            case IMM_RDTIMEH: {
              log_info("RDTIMEH");
              throw std::runtime_error("Insctruion not implemented yet.");
              break;
            }
            case IMM_RDINSTRET: {
              log_info("RDINSTRET");
              throw std::runtime_error("Insctruion not implemented yet.");
              break;
            }
            case IMM_RDINSTRETH: {
              log_info("RDINSTRETH");
              throw std::runtime_error("Insctruion not implemented yet.");
              break;
            }
            default: {
              log_error("[R2] Cannot decode instruction", inst.get_value());
              exit(1);
            }
          }
        }
        break;
      }
      default: {
        if (inst.get_value() != 0) {
          log_error("Cannot decode instruction", inst.get_value());
          exit(1);
        }
      }
    }
  }

public:
  RV32I() { }

  void load_to_ram(std::vector<uint32_t> data) {
    _ram.load(data);
  }

  void run() {
    int i = 0;
    while (i++ < 100000) {
      Instruction* inst = instruction_fetch();
      execute(*inst);
#ifdef REGDUMP
    _regs.dump_regs();
#endif
    }
  }
};

static uint32_t inline swapEndian(uint32_t value) {
    return ((value >> 24) & 0xFF) | ((value >> 8) & 0xFF00) |
           ((value << 8) & 0xFF0000) | ((value << 24) & 0xFF000000);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "[ERROR] Usage: ./main <filename>" << std::endl;
    exit(1);
  }
  const int ms = 1024 * 1024 * 4;
  std::ifstream file(argv[1], std::ios::binary);

  file.seekg(0, std::ios::end);
  std::streampos file_size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::size_t elems = file_size / sizeof(uint32_t);

  if (elems > ms ) {
    std::cout << file_size / sizeof(uint32_t) << " > " << ms << std::endl;
    exit(1);
  }

  std::vector<uint32_t> buffer(elems);

  file.read(reinterpret_cast<char*>(buffer.data()), file_size);
  file.close();
 
  // TODO: optimize this loop. It does run unnesseecrly if debug flag is not set.
  bool is_zero = false;
  int zeros = 0;
  for (uint32_t v: buffer) {
    if (v == 0) {
      is_zero = true;
      zeros++;
      continue;
    }
    if (is_zero) {
      is_zero = true;
      zeros++;
      log_debug_hex("value *", 0);
      continue;
    }
    log_debug_hex("value", v);
  }

  auto rv = new RV32I();
  rv->load_to_ram(buffer);
  rv->run();
  return 0;
}
