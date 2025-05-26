#include "cpu.hpp"

#include <cstdint>

#include "mmu.hpp"
#include "types.hpp"

namespace GbcEmulator
{


static constexpr std::array<Word, 32> interrupt_jump_table = {
    0x0000, 0x0040, 0x0048, 0x0040,
    0x0050, 0x0040, 0x0048, 0x0040,
    0x0058, 0x0040, 0x0048, 0x0040,
    0x0050, 0x0040, 0x0048, 0x0040,
    0x0060, 0x0040, 0x0048, 0x0040,
    0x0050, 0x0040, 0x0048, 0x0040,
    0x0058, 0x0040, 0x0048, 0x0040,
    0x0050, 0x0040, 0x0048, 0x0040,
};

static constexpr std::array<Byte, 32> interrupt_mask_table = {
    0xE0, 0xFE, 0xFC, 0xFE,
    0xF8, 0xFE, 0xFC, 0xFE,
    0xF0, 0xFE, 0xFC, 0xFE,
    0xF8, 0xFE, 0xFC, 0xFE,
    0xE0, 0xFE, 0xFC, 0xFE,
    0xF8, 0xFE, 0xFC, 0xFE,
    0xF0, 0xFE, 0xFC, 0xFE,
    0xF8, 0xFE, 0xFC, 0xFE
};

// TODO: Handle GBC double speed mode
void Cpu::stepTCycles(TCycleCount cycles)
{
    if (state_.paused)
        return;

    TCycleCount starting_time = clock_.get();
    TCycleCount target_cycle = starting_time + cycles;
    while (target_cycle > clock_.get())
    {     
        if (breakpoints_.test(state_[Reg16::PC])
        && starting_time != clock_.get())
        {
            state_.paused = true;
            return;
        }

        switch (state_.mode)
        {
        // TODO: recreate HALT bug
        case CpuState::Mode::Halted:
            if (bus_.load(0xFF0F) & bus_.load(0xFFFF) & 0x1F)
            {
                state_.mode = CpuState::Mode::Normal;
                break;
            }
            clock_.add(4);
            continue;
        
        // TODO: handle STOP instruction black magic
        case CpuState::Mode::Stopped:
            return;

        default:
            break;
        }

        if (state_.ime)
        {
            Byte interrupt = bus_.load(0xFF0F) & bus_.load(0xFFFF) & 0x1F;
            if (interrupt)
            {
                state_.ime = false;
                state_.next_ime = false;
                bus_.store(0xFF0F, interrupt & interrupt_mask_table[interrupt]);

                clock_.add(4);
                call(interrupt_jump_table[interrupt]);
                clock_.add(4);
            }
        }

        state_.ime = state_.next_ime;

        Byte op = readNextByte();
        baseInstruction(op);
    }
}

void Cpu::reset()
{
    clock_.reset();
    state_.reset();
}


Byte Cpu::readAtAddr(Word address)
{
    clock_.add(4);
    return bus_.load(address);
}


void Cpu::writeAtAddr(Word address, Byte value)
{
    clock_.add(4);
    bus_.store(address, value);
}


void Cpu::writeWordAtAddr(Word address, Word value)
{
    clock_.add(4);
    bus_.store(address, static_cast<Byte>(value & 0xFF));
    clock_.add(4);
    bus_.store(++address, static_cast<Byte>(value >> 8));
}


Byte Cpu::readNextByte()
{
    return readAtAddr(state_[Reg16::PC]++);
}


Word Cpu::readNextWord()
{
    Word value = readNextByte();
    value |= static_cast<Word>(static_cast<Word>(readNextByte()) << 8);
    return value;
}


void Cpu::stop()
{
    state_.mode = CpuState::Mode::Stopped;
    ++state_[Reg16::PC];
}


void Cpu::halt()
{
    state_.mode = CpuState::Mode::Halted;
}


void Cpu::ei()
{
    state_.next_ime = true;
}


void Cpu::di()
{
    state_.ime = false;
    state_.next_ime = false;
}


void Cpu::loadAddSigned(_Reg16 regTo, Word value, Byte offset)
{
    clock_.add(4);
    state_[Reg8::F] = 0;
    state_[RegFlag::C] = ((value & 0xFF) + offset > 0xFF);
    state_[RegFlag::H] = ((value & 0xF) + (offset & 0xF) > 0xF);
    // Cast to int8_t before int16_t, so sign extension works properly
    // Also, signed <=> unsigned static_cast is defined in C++20
    regTo = value + static_cast<Word>(static_cast<int16_t>(static_cast<int8_t>(offset)));
}


void Cpu::incAddr(_Reg16 addr)
{
    Byte r = readAtAddr(addr) + 1;
    writeAtAddr(addr, r);
    state_[RegFlag::H] = !(r & 0xF);
    state_[RegFlag::N] = false;
    state_[RegFlag::Z] = (r == 0);
}


void Cpu::decAddr(_Reg16 addr)
{
    Byte r = readAtAddr(addr);
    state_[RegFlag::H] = !(r & 0xF);
    writeAtAddr(addr, --r);
    state_[RegFlag::N] = true;
    state_[RegFlag::Z] = (r == 0);
}


void Cpu::incWord(_Reg16 reg)
{
    clock_.add(4);
    ++reg;
}


void Cpu::decWord(_Reg16 reg)
{
    clock_.add(4);
    --reg;
}


void Cpu::incByte(Byte& reg)
{
    ++reg;
    state_[RegFlag::H] = !(reg & 0xF);
    state_[RegFlag::N] = false;
    state_[RegFlag::Z] = (reg == 0);
}


void Cpu::decByte(Byte& reg)
{
    state_[RegFlag::H] = !(reg & 0xF);
    --reg;
    state_[RegFlag::Z] = (reg == 0);
    state_[RegFlag::N] = true;
}


void Cpu::addWord(_Reg16 regTo, Word value)
{
    clock_.add(4);
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = ((value & 0xFFF) + (regTo & 0xFFF) > 0xFFF);
    Word old_value = regTo;
    regTo += value;
    state_[RegFlag::C] = (old_value > regTo);
}


void Cpu::addWordSigned(_Reg16 regTo, Byte value)
{
    clock_.add(8);
    state_[Reg8::F] = 0;
    state_[RegFlag::C] = ((regTo & 0xFF) + value > 0xFF);
    state_[RegFlag::H] = ((regTo & 0xF) + (value & 0xF) > 0xF);
    // Cast to int8_t before int16_t, so sign extension works properly
    // Also, signed <=> unsigned static_cast is defined in C++20
    regTo += static_cast<Word>(static_cast<int16_t>(static_cast<int8_t>(value)));
}


void Cpu::add(Byte& regTo, Byte value)
{
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = ((value & 0xF) + (regTo & 0xF) > 0xF);
    Byte old_value = regTo;
    regTo += value;
    state_[RegFlag::C] = (old_value > regTo);
    state_[RegFlag::Z] = (regTo == 0);
}


void Cpu::adc(Byte& regTo, Byte value)
{
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = (state_[RegFlag::C] + (value & 0xF) + (regTo & 0xF) > 0xF);
    if (state_[RegFlag::C])
    {
        if (value == 0xFF)
        {
            state_[RegFlag::Z] = (regTo == 0);
            state_[RegFlag::C] = true;
            return;
        }
        ++value;
    }
    Byte old_value = regTo;
    regTo += value;
    state_[RegFlag::C] = (old_value > regTo);
    state_[RegFlag::Z] = (regTo == 0);
}


void Cpu::sub(Byte& regTo, Byte value)
{
    state_[RegFlag::N] = true;
    state_[RegFlag::H] = (regTo & 0xF) < (value & 0xF);
    state_[RegFlag::C] = regTo < value;
    regTo -= value;
    state_[RegFlag::Z] = (regTo == 0);
}


void Cpu::sbc(Byte& regTo, Byte value)
{
    state_[RegFlag::N] = true;
    state_[RegFlag::H] = (regTo & 0xF) < (value & 0xF) + state_[RegFlag::C];
    if (state_[RegFlag::C])
    {
        if (value == 0xFF)
        {
            state_[RegFlag::C] = true;
            state_[RegFlag::Z] = (regTo == 0);
            return;
        }
        ++value;
    }
    state_[RegFlag::C] = regTo < value;
    regTo -= value;
    state_[RegFlag::Z] = (regTo == 0);
}


void Cpu::bitwiseAnd(Byte& regTo, Byte value)
{
    regTo &= value;
    state_[RegFlag::Z] = (regTo == 0);
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = true;
    state_[RegFlag::C] = false;
}


void Cpu::bitwiseXor(Byte& regTo, Byte value)
{
    regTo ^= value;
    state_[RegFlag::Z] = (regTo == 0);
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = false;
    state_[RegFlag::C] = false;
}


void Cpu::bitwiseOr(Byte& regTo, Byte value)
{
    regTo |= value;
    state_[RegFlag::Z] = (regTo == 0);
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = false;
    state_[RegFlag::C] = false;
}


void Cpu::cp(Byte regA, Byte regB)
{
    state_[RegFlag::N] = true;
    state_[RegFlag::H] = (regA & 0xF) < (regB & 0xF);
    state_[RegFlag::C] = regA < regB;
    state_[RegFlag::Z] = (regA == regB);
}


void Cpu::rlca()
{
    state_[Reg8::F] = 0;
    state_[RegFlag::C] = (1 << 7) & state_[Reg8::A];
    state_[Reg8::A] = (state_[Reg8::A] << 1) | static_cast<Byte>(state_[RegFlag::C]);
}


void Cpu::rla()
{
    bool new_carry = state_[Reg8::A] & (1 << 7);
    state_[Reg8::A] = static_cast<Byte>(state_[RegFlag::C]) | (state_[Reg8::A] << 1);
    state_[Reg8::F] = 0;
    state_[RegFlag::C] = new_carry;
}


void Cpu::rrca()
{
    state_[Reg8::F] = 0;
    state_[RegFlag::C] = 1 & state_[Reg8::A];
    state_[Reg8::A] = (state_[Reg8::A] >> 1) | (state_[RegFlag::C] << 7);
}


void Cpu::rra()
{
    bool new_carry = state_[Reg8::A] & 1;
    state_[Reg8::A] = (state_[Reg8::A] >> 1) | (static_cast<Byte>(state_[RegFlag::C]) << 7);
    state_[Reg8::F] = 0;
    state_[RegFlag::C] = new_carry;
}


void Cpu::daa()
{
    if (state_[RegFlag::N])
    {
        // Sub
        if (state_[RegFlag::C])
        {
            state_[Reg8::A] -= 0x60;
        }
        if (state_[RegFlag::H])
        {
            state_[Reg8::A] -= 0x6;
        }
    }
    else
    {
        // Add
        if ((state_[Reg8::A] > 0x99) || state_[RegFlag::C])
        {
            state_[Reg8::A] += 0x60;
            state_[RegFlag::C] = true;
        }
        if (((state_[Reg8::A] & 0xF) > 0x9) || state_[RegFlag::H])
        {
            state_[Reg8::A] += 0x6;
        }
    }

    state_[RegFlag::H] = false;
    state_[RegFlag::Z] = (state_[Reg8::A] == 0);
}


void Cpu::cpl()
{
    state_[RegFlag::N] = true;
    state_[RegFlag::H] = true;
    state_[Reg8::A] ^= 0xFF;
}


void Cpu::scf()
{
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = false;
    state_[RegFlag::C] = true;
}


void Cpu::ccf()
{
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = false;
    state_[RegFlag::C] = !state_[RegFlag::C];
}



void Cpu::jmpRel(Byte offset, bool condition)
{
    if (!condition)
        return;

    clock_.add(4);
    
    // Cast to int8_t before int16_t, so sign extension works properly
    // Also, signed <=> unsigned static_cast is defined in C++20
    state_[Reg16::PC] += static_cast<Word>(static_cast<int16_t>(static_cast<int8_t>(offset)));
}


void Cpu::jmp(Word address, bool condition)
{
    if (!condition)
        return;

    clock_.add(4);
    state_[Reg16::PC] = address;
}


void Cpu::push(Word value)
{
    clock_.add(4);
    writeAtAddr(--state_[Reg16::SP], static_cast<Byte>(value >> 8));
    writeAtAddr(--state_[Reg16::SP], static_cast<Byte>(value & 0xFF));
}


void Cpu::pop(_Reg16 regTo)
{
    Word value = static_cast<Word>(readAtAddr(state_[Reg16::SP]++));
    value += static_cast<Word>(readAtAddr(state_[Reg16::SP]++) << 8);
    regTo = value;
}


void Cpu::call(Word address, bool condition)
{
    if (!condition)
        return;

    push(state_[Reg16::PC]);
    state_[Reg16::PC] = address;
}


void Cpu::ret()
{
    pop(state_[Reg16::PC]);
    clock_.add(4);
}


void Cpu::ret(bool condition)
{
    clock_.add(4);
    if (condition)
        ret();
}


void Cpu::reti()
{
    ret();
    state_.next_ime = true;
    state_.ime = true;
}


Byte Cpu::rlc(Byte value)
{
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = false;
    state_[RegFlag::C] = value & (1 << 7);
    Byte result = (value << 1) | static_cast<Byte>(state_[RegFlag::C]);
    state_[RegFlag::Z] = (result == 0);
    return result;
}


Byte Cpu::rrc(Byte value)
{
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = false;
    state_[RegFlag::C] = value & 1;
    Byte result = (value >> 1) | (static_cast<Byte>(state_[RegFlag::C]) << 7);
    state_[RegFlag::Z] = (result == 0);
    return result;
}


Byte Cpu::rl(Byte value)
{
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = false;
    bool new_carry = value & (1 << 7);
    Byte result = (value << 1) | static_cast<Byte>(state_[RegFlag::C]);
    state_[RegFlag::Z] = (result == 0);
    state_[RegFlag::C] = new_carry;
    return result;
}


Byte Cpu::rr(Byte value)
{
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = false;
    bool new_carry = value & 1;
    Byte result = (value >> 1) | (static_cast<Byte>(state_[RegFlag::C]) << 7);
    state_[RegFlag::Z] = (result == 0);
    state_[RegFlag::C] = new_carry;
    return result;
}


Byte Cpu::sla(Byte value)
{
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = false;
    state_[RegFlag::C] = value >> 7;
    Byte result = value << 1;
    state_[RegFlag::Z] = (result == 0);
    return result;
}


Byte Cpu::sra(Byte value)
{
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = false;
    state_[RegFlag::C] = value & 1;
    Byte result = (value & 0x80) | (value >> 1);
    state_[RegFlag::Z] = (result == 0);
    return result;
}


Byte Cpu::swap(Byte value)
{
    state_[RegFlag::Z] = (value == 0);
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = false;
    state_[RegFlag::C] = false;
    return (value >> 4) | (value << 4);
}


Byte Cpu::srl(Byte value)
{
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = false;
    state_[RegFlag::C] = value & 1;
    Byte result = value >> 1;
    state_[RegFlag::Z] = (result == 0);
    return result;
}


void Cpu::bit(Byte value, Byte bit)
{
    state_[RegFlag::N] = false;
    state_[RegFlag::H] = true;
    state_[RegFlag::Z] = !(value & (1 << bit));
}


Byte Cpu::res(Byte value, Byte bit)
{
    return value & static_cast<Byte>(~(1 << bit));
}


Byte Cpu::set(Byte value, Byte bit)
{
    return value | (1 << bit);
}


void Cpu::undefinedInstruction()
{
    state_.paused = true;
    // TODO: log or crash or something
}

void Cpu::baseInstruction(Byte inst)
{
    switch (inst)
    {
        // ====== First misc block (0x00 - 0x3F) ======

        case 0x00: break; // no op
        case 0x01: state_[Reg16::BC] = readNextWord(); break;
        case 0x02: writeAtAddr(state_[Reg16::BC], state_[Reg8::A]); break;
        case 0x03: incWord(state_[Reg16::BC]); break;
        case 0x04: incByte(state_[Reg8::B]); break;
        case 0x05: decByte(state_[Reg8::B]); break;
        case 0x06: state_[Reg8::B] = readNextByte(); break;
        case 0x07: rlca(); break;
        case 0x08: writeWordAtAddr(readNextWord(), state_[Reg16::SP]); break;
        case 0x09: addWord(state_[Reg16::HL], state_[Reg16::BC]); break;
        case 0x0A: state_[Reg8::A] = readAtAddr(state_[Reg16::BC]); break;
        case 0x0B: decWord(state_[Reg16::BC]); break;
        case 0x0C: incByte(state_[Reg8::C]); break;
        case 0x0D: decByte(state_[Reg8::C]); break;
        case 0x0E: state_[Reg8::C] = readNextByte(); break;
        case 0x0F: rrca(); break;

        case 0x10: stop(); break;
        case 0x11: state_[Reg16::DE] = readNextWord(); break;
        case 0x12: writeAtAddr(state_[Reg16::DE], state_[Reg8::A]); break;
        case 0x13: incWord(state_[Reg16::DE]); break;
        case 0x14: incByte(state_[Reg8::D]); break;
        case 0x15: decByte(state_[Reg8::D]); break;
        case 0x16: state_[Reg8::D] = readNextByte(); break;
        case 0x17: rla(); break;
        case 0x18: jmpRel(readNextByte()); break;
        case 0x19: addWord(state_[Reg16::HL], state_[Reg16::DE]); break;
        case 0x1A: state_[Reg8::A] = readAtAddr(state_[Reg16::DE]); break;
        case 0x1B: decWord(state_[Reg16::DE]); break;
        case 0x1C: incByte(state_[Reg8::E]); break;
        case 0x1D: decByte(state_[Reg8::E]); break;
        case 0x1E: state_[Reg8::E] = readNextByte(); break;
        case 0x1F: rra(); break;

        case 0x20: jmpRel(readNextByte(), !state_[RegFlag::Z]); break;
        case 0x21: state_[Reg16::HL] = readNextWord(); break;
        case 0x22: writeAtAddr(state_[Reg16::HL]++, state_[Reg8::A]); break;
        case 0x23: incWord(state_[Reg16::HL]); break;
        case 0x24: incByte(state_[Reg8::H]); break;
        case 0x25: decByte(state_[Reg8::H]); break;
        case 0x26: state_[Reg8::H] = readNextByte(); break;
        case 0x27: daa(); break;
        case 0x28: jmpRel(readNextByte(), state_[RegFlag::Z]); break;
        case 0x29: addWord(state_[Reg16::HL], state_[Reg16::HL]); break;
        case 0x2A: state_[Reg8::A] = readAtAddr(state_[Reg16::HL]++); break;
        case 0x2B: decWord(state_[Reg16::HL]); break;
        case 0x2C: incByte(state_[Reg8::L]); break;
        case 0x2D: decByte(state_[Reg8::L]); break;
        case 0x2E: state_[Reg8::L] = readNextByte(); break;
        case 0x2F: cpl(); break;

        case 0x30: jmpRel(readNextByte(), !state_[RegFlag::C]); break;
        case 0x31: state_[Reg16::SP] = readNextWord(); break;
        case 0x32: writeAtAddr(state_[Reg16::HL]--, state_[Reg8::A]); break;
        case 0x33: incWord(state_[Reg16::SP]); break;
        case 0x34: incAddr(state_[Reg16::HL]); break;
        case 0x35: decAddr(state_[Reg16::HL]); break;
        case 0x36: writeAtAddr(state_[Reg16::HL], readNextByte()); break; 
        case 0x37: scf(); break;
        case 0x38: jmpRel(readNextByte(), state_[RegFlag::C]); break;
        case 0x39: addWord(state_[Reg16::HL], state_[Reg16::SP]); break;
        case 0x3A: state_[Reg8::A] = readAtAddr(state_[Reg16::HL]--); break;
        case 0x3B: decWord(state_[Reg16::SP]); break;
        case 0x3C: incByte(state_[Reg8::A]); break;
        case 0x3D: decByte(state_[Reg8::A]); break;
        case 0x3E: state_[Reg8::A] = readNextByte(); break;
        case 0x3F: ccf(); break;

        // ====== Load block (0x40 - 0x7F) ======

        case 0x40: state_[Reg8::B] = state_[Reg8::B]; break;
        case 0x41: state_[Reg8::B] = state_[Reg8::C]; break;
        case 0x42: state_[Reg8::B] = state_[Reg8::D]; break;
        case 0x43: state_[Reg8::B] = state_[Reg8::E]; break;
        case 0x44: state_[Reg8::B] = state_[Reg8::H]; break;
        case 0x45: state_[Reg8::B] = state_[Reg8::L]; break;
        case 0x46: state_[Reg8::B] = readAtAddr(state_[Reg16::HL]); break;
        case 0x47: state_[Reg8::B] = state_[Reg8::A]; break;
        case 0x48: state_[Reg8::C] = state_[Reg8::B]; break;
        case 0x49: state_[Reg8::C] = state_[Reg8::C]; break;
        case 0x4A: state_[Reg8::C] = state_[Reg8::D]; break;
        case 0x4B: state_[Reg8::C] = state_[Reg8::E]; break;
        case 0x4C: state_[Reg8::C] = state_[Reg8::H]; break;
        case 0x4D: state_[Reg8::C] = state_[Reg8::L]; break;
        case 0x4E: state_[Reg8::C] = readAtAddr(state_[Reg16::HL]); break;
        case 0x4F: state_[Reg8::C] = state_[Reg8::A]; break;

        case 0x51: state_[Reg8::D] = state_[Reg8::C]; break;
        case 0x50: state_[Reg8::D] = state_[Reg8::B]; break;
        case 0x52: state_[Reg8::D] = state_[Reg8::D]; break;
        case 0x53: state_[Reg8::D] = state_[Reg8::E]; break;
        case 0x54: state_[Reg8::D] = state_[Reg8::H]; break;
        case 0x55: state_[Reg8::D] = state_[Reg8::L]; break;
        case 0x56: state_[Reg8::D] = readAtAddr(state_[Reg16::HL]); break;
        case 0x57: state_[Reg8::D] = state_[Reg8::A]; break;
        case 0x58: state_[Reg8::E] = state_[Reg8::B]; break;
        case 0x59: state_[Reg8::E] = state_[Reg8::C]; break;
        case 0x5A: state_[Reg8::E] = state_[Reg8::D]; break;
        case 0x5B: state_[Reg8::E] = state_[Reg8::E]; break;
        case 0x5C: state_[Reg8::E] = state_[Reg8::H]; break;
        case 0x5D: state_[Reg8::E] = state_[Reg8::L]; break;
        case 0x5E: state_[Reg8::E] = readAtAddr(state_[Reg16::HL]); break;
        case 0x5F: state_[Reg8::E] = state_[Reg8::A]; break;

        case 0x60: state_[Reg8::H] = state_[Reg8::B]; break;
        case 0x61: state_[Reg8::H] = state_[Reg8::C]; break;
        case 0x62: state_[Reg8::H] = state_[Reg8::D]; break;
        case 0x63: state_[Reg8::H] = state_[Reg8::E]; break;
        case 0x64: state_[Reg8::H] = state_[Reg8::H]; break;
        case 0x65: state_[Reg8::H] = state_[Reg8::L]; break;
        case 0x66: state_[Reg8::H] = readAtAddr(state_[Reg16::HL]); break;
        case 0x67: state_[Reg8::H] = state_[Reg8::A]; break;
        case 0x68: state_[Reg8::L] = state_[Reg8::B]; break;
        case 0x69: state_[Reg8::L] = state_[Reg8::C]; break;
        case 0x6A: state_[Reg8::L] = state_[Reg8::D]; break;
        case 0x6B: state_[Reg8::L] = state_[Reg8::E]; break;
        case 0x6C: state_[Reg8::L] = state_[Reg8::H]; break;
        case 0x6D: state_[Reg8::L] = state_[Reg8::L]; break;
        case 0x6E: state_[Reg8::L] = readAtAddr(state_[Reg16::HL]); break;
        case 0x6F: state_[Reg8::L] = state_[Reg8::A]; break;

        case 0x70: writeAtAddr(state_[Reg16::HL], state_[Reg8::B]); break;
        case 0x71: writeAtAddr(state_[Reg16::HL], state_[Reg8::C]); break;
        case 0x72: writeAtAddr(state_[Reg16::HL], state_[Reg8::D]); break;
        case 0x73: writeAtAddr(state_[Reg16::HL], state_[Reg8::E]); break;
        case 0x74: writeAtAddr(state_[Reg16::HL], state_[Reg8::H]); break;
        case 0x75: writeAtAddr(state_[Reg16::HL], state_[Reg8::L]); break;
        case 0x76: halt(); break;
        case 0x77: writeAtAddr(state_[Reg16::HL], state_[Reg8::A]); break;
        case 0x78: state_[Reg8::A] = state_[Reg8::B]; break;
        case 0x79: state_[Reg8::A] = state_[Reg8::C]; break;
        case 0x7A: state_[Reg8::A] = state_[Reg8::D]; break;
        case 0x7B: state_[Reg8::A] = state_[Reg8::E]; break;
        case 0x7C: state_[Reg8::A] = state_[Reg8::H]; break;
        case 0x7D: state_[Reg8::A] = state_[Reg8::L]; break;
        case 0x7E: state_[Reg8::A] = readAtAddr(state_[Reg16::HL]); break;
        case 0x7F: state_[Reg8::A] = state_[Reg8::A]; break;

        // ====== Arithmetic block (0x80 - 0xBF) ======
        
        case 0x80: add(state_[Reg8::A], state_[Reg8::B]); break;
        case 0x81: add(state_[Reg8::A], state_[Reg8::C]); break;
        case 0x82: add(state_[Reg8::A], state_[Reg8::D]); break;
        case 0x83: add(state_[Reg8::A], state_[Reg8::E]); break;
        case 0x84: add(state_[Reg8::A], state_[Reg8::H]); break;
        case 0x85: add(state_[Reg8::A], state_[Reg8::L]); break;
        case 0x86: add(state_[Reg8::A], readAtAddr(state_[Reg16::HL])); break;
        case 0x87: add(state_[Reg8::A], state_[Reg8::A]); break;
        case 0x88: adc(state_[Reg8::A], state_[Reg8::B]); break;
        case 0x89: adc(state_[Reg8::A], state_[Reg8::C]); break;
        case 0x8A: adc(state_[Reg8::A], state_[Reg8::D]); break;
        case 0x8B: adc(state_[Reg8::A], state_[Reg8::E]); break;
        case 0x8C: adc(state_[Reg8::A], state_[Reg8::H]); break;
        case 0x8D: adc(state_[Reg8::A], state_[Reg8::L]); break;
        case 0x8E: adc(state_[Reg8::A], readAtAddr(state_[Reg16::HL])); break;
        case 0x8F: adc(state_[Reg8::A], state_[Reg8::A]); break;

        case 0x90: sub(state_[Reg8::A], state_[Reg8::B]); break;
        case 0x91: sub(state_[Reg8::A], state_[Reg8::C]); break;
        case 0x92: sub(state_[Reg8::A], state_[Reg8::D]); break;
        case 0x93: sub(state_[Reg8::A], state_[Reg8::E]); break;
        case 0x94: sub(state_[Reg8::A], state_[Reg8::H]); break;
        case 0x95: sub(state_[Reg8::A], state_[Reg8::L]); break;
        case 0x96: sub(state_[Reg8::A], readAtAddr(state_[Reg16::HL])); break;
        case 0x97: sub(state_[Reg8::A], state_[Reg8::A]); break;
        case 0x98: sbc(state_[Reg8::A], state_[Reg8::B]); break;
        case 0x99: sbc(state_[Reg8::A], state_[Reg8::C]); break;
        case 0x9A: sbc(state_[Reg8::A], state_[Reg8::D]); break;
        case 0x9B: sbc(state_[Reg8::A], state_[Reg8::E]); break;
        case 0x9C: sbc(state_[Reg8::A], state_[Reg8::H]); break;
        case 0x9D: sbc(state_[Reg8::A], state_[Reg8::L]); break;
        case 0x9E: sbc(state_[Reg8::A], readAtAddr(state_[Reg16::HL])); break;
        case 0x9F: sbc(state_[Reg8::A], state_[Reg8::A]); break;

        case 0xA0: bitwiseAnd(state_[Reg8::A], state_[Reg8::B]); break;
        case 0xA1: bitwiseAnd(state_[Reg8::A], state_[Reg8::C]); break;
        case 0xA2: bitwiseAnd(state_[Reg8::A], state_[Reg8::D]); break;
        case 0xA3: bitwiseAnd(state_[Reg8::A], state_[Reg8::E]); break;
        case 0xA4: bitwiseAnd(state_[Reg8::A], state_[Reg8::H]); break;
        case 0xA5: bitwiseAnd(state_[Reg8::A], state_[Reg8::L]); break;
        case 0xA6: bitwiseAnd(state_[Reg8::A], readAtAddr(state_[Reg16::HL])); break;
        case 0xA7: bitwiseAnd(state_[Reg8::A], state_[Reg8::A]); break;
        case 0xA8: bitwiseXor(state_[Reg8::A], state_[Reg8::B]); break;
        case 0xA9: bitwiseXor(state_[Reg8::A], state_[Reg8::C]); break;
        case 0xAA: bitwiseXor(state_[Reg8::A], state_[Reg8::D]); break;
        case 0xAB: bitwiseXor(state_[Reg8::A], state_[Reg8::E]); break;
        case 0xAC: bitwiseXor(state_[Reg8::A], state_[Reg8::H]); break;
        case 0xAD: bitwiseXor(state_[Reg8::A], state_[Reg8::L]); break;
        case 0xAE: bitwiseXor(state_[Reg8::A], readAtAddr(state_[Reg16::HL])); break;
        case 0xAF: bitwiseXor(state_[Reg8::A], state_[Reg8::A]); break;

        case 0xB0: bitwiseOr(state_[Reg8::A], state_[Reg8::B]); break;
        case 0xB1: bitwiseOr(state_[Reg8::A], state_[Reg8::C]); break;
        case 0xB2: bitwiseOr(state_[Reg8::A], state_[Reg8::D]); break;
        case 0xB3: bitwiseOr(state_[Reg8::A], state_[Reg8::E]); break;
        case 0xB4: bitwiseOr(state_[Reg8::A], state_[Reg8::H]); break;
        case 0xB5: bitwiseOr(state_[Reg8::A], state_[Reg8::L]); break;
        case 0xB6: bitwiseOr(state_[Reg8::A], readAtAddr(state_[Reg16::HL])); break;
        case 0xB7: bitwiseOr(state_[Reg8::A], state_[Reg8::A]); break;
        case 0xB8: cp(state_[Reg8::A], state_[Reg8::B]); break;
        case 0xB9: cp(state_[Reg8::A], state_[Reg8::C]); break;
        case 0xBA: cp(state_[Reg8::A], state_[Reg8::D]); break;
        case 0xBB: cp(state_[Reg8::A], state_[Reg8::E]); break;
        case 0xBC: cp(state_[Reg8::A], state_[Reg8::H]); break;
        case 0xBD: cp(state_[Reg8::A], state_[Reg8::L]); break;
        case 0xBE: cp(state_[Reg8::A], readAtAddr(state_[Reg16::HL])); break;
        case 0xBF: cp(state_[Reg8::A], state_[Reg8::A]); break;

        // ====== Last misc block (0xC0 - 0xFF) ======

        case 0xC0: ret(!state_[RegFlag::Z]); break;
        case 0xC1: pop(state_[Reg16::BC]); break;
        case 0xC2: jmp(readNextWord(), !state_[RegFlag::Z]); break;
        case 0xC3: jmp(readNextWord()); break;
        case 0xC4: call(readNextWord(), !state_[RegFlag::Z]); break;
        case 0xC5: push(state_[Reg16::BC]); break;
        case 0xC6: add(state_[Reg8::A], readNextByte()); break;
        case 0xC7: call(0x0000); break;
        case 0xC8: ret(state_[RegFlag::Z]); break;
        case 0xC9: ret(); break;
        case 0xCA: jmp(readNextWord(), state_[RegFlag::Z]); break;
        case 0xCB: prefixInstruction(readNextByte()); break;
        case 0xCC: call(readNextWord(), state_[RegFlag::Z]); break;
        case 0xCD: call(readNextWord()); break;
        case 0xCE: adc(state_[Reg8::A], readNextByte()); break;
        case 0xCF: call(0x0008); break;

        case 0xD0: ret(!state_[RegFlag::C]); break;
        case 0xD1: pop(state_[Reg16::DE]); break;
        case 0xD2: jmp(readNextWord(), !state_[RegFlag::C]); break;
        case 0xD3: undefinedInstruction(); break;
        case 0xD4: call(readNextWord(), !state_[RegFlag::C]); break;
        case 0xD5: push(state_[Reg16::DE]); break;
        case 0xD6: sub(state_[Reg8::A], readNextByte()); break;
        case 0xD7: call(0x0010); break;
        case 0xD8: ret(state_[RegFlag::C]); break;
        case 0xD9: reti(); break;
        case 0xDA: jmp(readNextWord(), state_[RegFlag::C]); break;
        case 0xDB: undefinedInstruction(); break;
        case 0xDC: call(readNextWord(), state_[RegFlag::C]); break;
        case 0xDD: undefinedInstruction(); break;
        case 0xDE: sbc(state_[Reg8::A], readNextByte()); break;
        case 0xDF: call(0x0018); break;

        case 0xE0: writeAtAddr(static_cast<Word>(readNextByte()) | 0xFF00, state_[Reg8::A]); break;
        case 0xE1: pop(state_[Reg16::HL]); break;
        case 0xE2: writeAtAddr(state_[Reg8::C] | 0xFF00, state_[Reg8::A]); break;
        case 0xE3: undefinedInstruction(); break;
        case 0xE4: undefinedInstruction(); break;
        case 0xE5: push(state_[Reg16::HL]); break;
        case 0xE6: bitwiseAnd(state_[Reg8::A], readNextByte()); break;
        case 0xE7: call(0x0020); break;
        case 0xE8: addWordSigned(state_[Reg16::SP], readNextByte()); break;
        case 0xE9: state_[Reg16::PC] = static_cast<Word>(state_[Reg16::HL]); break;
        case 0xEA: writeAtAddr(readNextWord(), state_[Reg8::A]); break;
        case 0xEB: undefinedInstruction(); break;
        case 0xEC: undefinedInstruction(); break;
        case 0xED: undefinedInstruction(); break;
        case 0xEE: bitwiseXor(state_[Reg8::A], readNextByte()); break;
        case 0xEF: call(0x0028); break;

        case 0xF0: state_[Reg8::A] = readAtAddr(static_cast<Word>(readNextByte()) | 0xFF00); break;
        case 0xF1: pop(state_[Reg16::AF]); state_[Reg8::F] &= 0xF0; break;
        case 0xF2: state_[Reg8::A] = readAtAddr(state_[Reg8::C] | 0xFF00); break;
        case 0xF3: di(); break;
        case 0xF4: undefinedInstruction(); break;
        case 0xF5: push(state_[Reg16::AF]); break;
        case 0xF6: bitwiseOr(state_[Reg8::A], readNextByte()); break;
        case 0xF7: call(0x0030); break;
        case 0xF8: loadAddSigned(state_[Reg16::HL], state_[Reg16::SP], readNextByte()); break;
        case 0xF9: state_[Reg16::SP] = static_cast<Word>(state_[Reg16::HL]); clock_.add(4); break;
        case 0xFA: state_[Reg8::A] = readAtAddr(readNextWord()); break;
        case 0xFB: ei(); break;
        case 0xFC: undefinedInstruction(); break;
        case 0xFD: undefinedInstruction(); break;
        case 0xFE: cp(state_[Reg8::A], readNextByte()); break;
        case 0xFF: call(0x0038); break;

        default: break;
    }
}

void Cpu::prefixInstruction(Byte inst)
{
    switch(inst)
    {
        // ====== Bit shifting block (0x00 - 0x3F) ======

        case 0x00: state_[Reg8::B] = rlc(state_[Reg8::B]); break;
        case 0x01: state_[Reg8::C] = rlc(state_[Reg8::C]); break;
        case 0x02: state_[Reg8::D] = rlc(state_[Reg8::D]); break;
        case 0x03: state_[Reg8::E] = rlc(state_[Reg8::E]); break;
        case 0x04: state_[Reg8::H] = rlc(state_[Reg8::H]); break;
        case 0x05: state_[Reg8::L] = rlc(state_[Reg8::L]); break;
        case 0x06: writeAtAddr(state_[Reg16::HL], rlc(readAtAddr(state_[Reg16::HL]))); break;
        case 0x07: state_[Reg8::A] = rlc(state_[Reg8::A]); break;
        case 0x08: state_[Reg8::B] = rrc(state_[Reg8::B]); break;
        case 0x09: state_[Reg8::C] = rrc(state_[Reg8::C]); break;
        case 0x0A: state_[Reg8::D] = rrc(state_[Reg8::D]); break;
        case 0x0B: state_[Reg8::E] = rrc(state_[Reg8::E]); break;
        case 0x0C: state_[Reg8::H] = rrc(state_[Reg8::H]); break;
        case 0x0D: state_[Reg8::L] = rrc(state_[Reg8::L]); break;
        case 0x0E: writeAtAddr(state_[Reg16::HL], rrc(readAtAddr(state_[Reg16::HL]))); break;
        case 0x0F: state_[Reg8::A] = rrc(state_[Reg8::A]); break;

        case 0x10: state_[Reg8::B] = rl(state_[Reg8::B]); break;
        case 0x11: state_[Reg8::C] = rl(state_[Reg8::C]); break;
        case 0x12: state_[Reg8::D] = rl(state_[Reg8::D]); break;
        case 0x13: state_[Reg8::E] = rl(state_[Reg8::E]); break;
        case 0x14: state_[Reg8::H] = rl(state_[Reg8::H]); break;
        case 0x15: state_[Reg8::L] = rl(state_[Reg8::L]); break;
        case 0x16: writeAtAddr(state_[Reg16::HL], rl(readAtAddr(state_[Reg16::HL]))); break;
        case 0x17: state_[Reg8::A] = rl(state_[Reg8::A]); break;
        case 0x18: state_[Reg8::B] = rr(state_[Reg8::B]); break;
        case 0x19: state_[Reg8::C] = rr(state_[Reg8::C]); break;
        case 0x1A: state_[Reg8::D] = rr(state_[Reg8::D]); break;
        case 0x1B: state_[Reg8::E] = rr(state_[Reg8::E]); break;
        case 0x1C: state_[Reg8::H] = rr(state_[Reg8::H]); break;
        case 0x1D: state_[Reg8::L] = rr(state_[Reg8::L]); break;
        case 0x1E: writeAtAddr(state_[Reg16::HL], rr(readAtAddr(state_[Reg16::HL]))); break;
        case 0x1F: state_[Reg8::A] = rr(state_[Reg8::A]); break;

        case 0x20: state_[Reg8::B] = sla(state_[Reg8::B]); break;
        case 0x21: state_[Reg8::C] = sla(state_[Reg8::C]); break;
        case 0x22: state_[Reg8::D] = sla(state_[Reg8::D]); break;
        case 0x23: state_[Reg8::E] = sla(state_[Reg8::E]); break;
        case 0x24: state_[Reg8::H] = sla(state_[Reg8::H]); break;
        case 0x25: state_[Reg8::L] = sla(state_[Reg8::L]); break;
        case 0x26: writeAtAddr(state_[Reg16::HL], sla(readAtAddr(state_[Reg16::HL]))); break;
        case 0x27: state_[Reg8::A] = sla(state_[Reg8::A]); break;
        case 0x28: state_[Reg8::B] = sra(state_[Reg8::B]); break;
        case 0x29: state_[Reg8::C] = sra(state_[Reg8::C]); break;
        case 0x2A: state_[Reg8::D] = sra(state_[Reg8::D]); break;
        case 0x2B: state_[Reg8::E] = sra(state_[Reg8::E]); break;
        case 0x2C: state_[Reg8::H] = sra(state_[Reg8::H]); break;
        case 0x2D: state_[Reg8::L] = sra(state_[Reg8::L]); break;
        case 0x2E: writeAtAddr(state_[Reg16::HL], sra(readAtAddr(state_[Reg16::HL]))); break;
        case 0x2F: state_[Reg8::A] = sra(state_[Reg8::A]); break;

        case 0x30: state_[Reg8::B] = swap(state_[Reg8::B]); break;
        case 0x31: state_[Reg8::C] = swap(state_[Reg8::C]); break;
        case 0x32: state_[Reg8::D] = swap(state_[Reg8::D]); break;
        case 0x33: state_[Reg8::E] = swap(state_[Reg8::E]); break;
        case 0x34: state_[Reg8::H] = swap(state_[Reg8::H]); break;
        case 0x35: state_[Reg8::L] = swap(state_[Reg8::L]); break;
        case 0x36: writeAtAddr(state_[Reg16::HL], swap(readAtAddr(state_[Reg16::HL]))); break;
        case 0x37: state_[Reg8::A] = swap(state_[Reg8::A]); break;
        case 0x38: state_[Reg8::B] = srl(state_[Reg8::B]); break;
        case 0x39: state_[Reg8::C] = srl(state_[Reg8::C]); break;
        case 0x3A: state_[Reg8::D] = srl(state_[Reg8::D]); break;
        case 0x3B: state_[Reg8::E] = srl(state_[Reg8::E]); break;
        case 0x3C: state_[Reg8::H] = srl(state_[Reg8::H]); break;
        case 0x3D: state_[Reg8::L] = srl(state_[Reg8::L]); break;
        case 0x3E: writeAtAddr(state_[Reg16::HL], srl(readAtAddr(state_[Reg16::HL]))); break;
        case 0x3F: state_[Reg8::A] = srl(state_[Reg8::A]); break;

        // ====== Bit check block (0x40 - 0x7F) ======

        case 0x40: bit(state_[Reg8::B], 0); break;
        case 0x41: bit(state_[Reg8::C], 0); break;
        case 0x42: bit(state_[Reg8::D], 0); break;
        case 0x43: bit(state_[Reg8::E], 0); break;
        case 0x44: bit(state_[Reg8::H], 0); break;
        case 0x45: bit(state_[Reg8::L], 0); break;
        case 0x46: bit(readAtAddr(state_[Reg16::HL]), 0); break;
        case 0x47: bit(state_[Reg8::A], 0); break;
        case 0x48: bit(state_[Reg8::B], 1); break;
        case 0x49: bit(state_[Reg8::C], 1); break;
        case 0x4A: bit(state_[Reg8::D], 1); break;
        case 0x4B: bit(state_[Reg8::E], 1); break;
        case 0x4C: bit(state_[Reg8::H], 1); break;
        case 0x4D: bit(state_[Reg8::L], 1); break;
        case 0x4E: bit(readAtAddr(state_[Reg16::HL]), 1); break;
        case 0x4F: bit(state_[Reg8::A], 1); break;

        case 0x50: bit(state_[Reg8::B], 2); break;
        case 0x51: bit(state_[Reg8::C], 2); break;
        case 0x52: bit(state_[Reg8::D], 2); break;
        case 0x53: bit(state_[Reg8::E], 2); break;
        case 0x54: bit(state_[Reg8::H], 2); break;
        case 0x55: bit(state_[Reg8::L], 2); break;
        case 0x56: bit(readAtAddr(state_[Reg16::HL]), 2); break;
        case 0x57: bit(state_[Reg8::A], 2); break;
        case 0x58: bit(state_[Reg8::B], 3); break;
        case 0x59: bit(state_[Reg8::C], 3); break;
        case 0x5A: bit(state_[Reg8::D], 3); break;
        case 0x5B: bit(state_[Reg8::E], 3); break;
        case 0x5C: bit(state_[Reg8::H], 3); break;
        case 0x5D: bit(state_[Reg8::L], 3); break;
        case 0x5E: bit(readAtAddr(state_[Reg16::HL]), 3); break;
        case 0x5F: bit(state_[Reg8::A], 3); break;
        
        case 0x60: bit(state_[Reg8::B], 4); break;
        case 0x61: bit(state_[Reg8::C], 4); break;
        case 0x62: bit(state_[Reg8::D], 4); break;
        case 0x63: bit(state_[Reg8::E], 4); break;
        case 0x64: bit(state_[Reg8::H], 4); break;
        case 0x65: bit(state_[Reg8::L], 4); break;
        case 0x66: bit(readAtAddr(state_[Reg16::HL]), 4); break;
        case 0x67: bit(state_[Reg8::A], 4); break;
        case 0x68: bit(state_[Reg8::B], 5); break;
        case 0x69: bit(state_[Reg8::C], 5); break;
        case 0x6A: bit(state_[Reg8::D], 5); break;
        case 0x6B: bit(state_[Reg8::E], 5); break;
        case 0x6C: bit(state_[Reg8::H], 5); break;
        case 0x6D: bit(state_[Reg8::L], 5); break;
        case 0x6E: bit(readAtAddr(state_[Reg16::HL]), 5); break;
        case 0x6F: bit(state_[Reg8::A], 5); break;

        case 0x70: bit(state_[Reg8::B], 6); break;
        case 0x71: bit(state_[Reg8::C], 6); break;
        case 0x72: bit(state_[Reg8::D], 6); break;
        case 0x73: bit(state_[Reg8::E], 6); break;
        case 0x74: bit(state_[Reg8::H], 6); break;
        case 0x75: bit(state_[Reg8::L], 6); break;
        case 0x76: bit(readAtAddr(state_[Reg16::HL]), 6); break;
        case 0x77: bit(state_[Reg8::A], 6); break;
        case 0x78: bit(state_[Reg8::B], 7); break;
        case 0x79: bit(state_[Reg8::C], 7); break;
        case 0x7A: bit(state_[Reg8::D], 7); break;
        case 0x7B: bit(state_[Reg8::E], 7); break;
        case 0x7C: bit(state_[Reg8::H], 7); break;
        case 0x7D: bit(state_[Reg8::L], 7); break;
        case 0x7E: bit(readAtAddr(state_[Reg16::HL]), 7); break;
        case 0x7F: bit(state_[Reg8::A], 7); break;

        // ====== Bit reset block (0x80 - 0xBF) ======

        case 0x80: state_[Reg8::B] = res(state_[Reg8::B], 0); break;
        case 0x81: state_[Reg8::C] = res(state_[Reg8::C], 0); break;
        case 0x82: state_[Reg8::D] = res(state_[Reg8::D], 0); break;
        case 0x83: state_[Reg8::E] = res(state_[Reg8::E], 0); break;
        case 0x84: state_[Reg8::H] = res(state_[Reg8::H], 0); break;
        case 0x85: state_[Reg8::L] = res(state_[Reg8::L], 0); break;
        case 0x86: writeAtAddr(state_[Reg16::HL], res(readAtAddr(state_[Reg16::HL]), 0)); break;
        case 0x87: state_[Reg8::A] = res(state_[Reg8::A], 0); break;
        case 0x88: state_[Reg8::B] = res(state_[Reg8::B], 1); break;
        case 0x89: state_[Reg8::C] = res(state_[Reg8::C], 1); break;
        case 0x8A: state_[Reg8::D] = res(state_[Reg8::D], 1); break;
        case 0x8B: state_[Reg8::E] = res(state_[Reg8::E], 1); break;
        case 0x8C: state_[Reg8::H] = res(state_[Reg8::H], 1); break;
        case 0x8D: state_[Reg8::L] = res(state_[Reg8::L], 1); break;
        case 0x8E: writeAtAddr(state_[Reg16::HL], res(readAtAddr(state_[Reg16::HL]), 1)); break;
        case 0x8F: state_[Reg8::A] = res(state_[Reg8::A], 1); break;

        case 0x90: state_[Reg8::B] = res(state_[Reg8::B], 2); break;
        case 0x91: state_[Reg8::C] = res(state_[Reg8::C], 2); break;
        case 0x92: state_[Reg8::D] = res(state_[Reg8::D], 2); break;
        case 0x93: state_[Reg8::E] = res(state_[Reg8::E], 2); break;
        case 0x94: state_[Reg8::H] = res(state_[Reg8::H], 2); break;
        case 0x95: state_[Reg8::L] = res(state_[Reg8::L], 2); break;
        case 0x96: writeAtAddr(state_[Reg16::HL], res(readAtAddr(state_[Reg16::HL]), 2)); break;
        case 0x97: state_[Reg8::A] = res(state_[Reg8::A], 2); break;
        case 0x98: state_[Reg8::B] = res(state_[Reg8::B], 3); break;
        case 0x99: state_[Reg8::C] = res(state_[Reg8::C], 3); break;
        case 0x9A: state_[Reg8::D] = res(state_[Reg8::D], 3); break;
        case 0x9B: state_[Reg8::E] = res(state_[Reg8::E], 3); break;
        case 0x9C: state_[Reg8::H] = res(state_[Reg8::H], 3); break;
        case 0x9D: state_[Reg8::L] = res(state_[Reg8::L], 3); break;
        case 0x9E: writeAtAddr(state_[Reg16::HL], res(readAtAddr(state_[Reg16::HL]), 3)); break;
        case 0x9F: state_[Reg8::A] = res(state_[Reg8::A], 3); break;
        
        case 0xA0: state_[Reg8::B] = res(state_[Reg8::B], 4); break;
        case 0xA1: state_[Reg8::C] = res(state_[Reg8::C], 4); break;
        case 0xA2: state_[Reg8::D] = res(state_[Reg8::D], 4); break;
        case 0xA3: state_[Reg8::E] = res(state_[Reg8::E], 4); break;
        case 0xA4: state_[Reg8::H] = res(state_[Reg8::H], 4); break;
        case 0xA5: state_[Reg8::L] = res(state_[Reg8::L], 4); break;
        case 0xA6: writeAtAddr(state_[Reg16::HL], res(readAtAddr(state_[Reg16::HL]), 4)); break;
        case 0xA7: state_[Reg8::A] = res(state_[Reg8::A], 4); break;
        case 0xA8: state_[Reg8::B] = res(state_[Reg8::B], 5); break;
        case 0xA9: state_[Reg8::C] = res(state_[Reg8::C], 5); break;
        case 0xAA: state_[Reg8::D] = res(state_[Reg8::D], 5); break;
        case 0xAB: state_[Reg8::E] = res(state_[Reg8::E], 5); break;
        case 0xAC: state_[Reg8::H] = res(state_[Reg8::H], 5); break;
        case 0xAD: state_[Reg8::L] = res(state_[Reg8::L], 5); break;
        case 0xAE: writeAtAddr(state_[Reg16::HL], res(readAtAddr(state_[Reg16::HL]), 5)); break;
        case 0xAF: state_[Reg8::A] = res(state_[Reg8::A], 5); break;

        case 0xB0: state_[Reg8::B] = res(state_[Reg8::B], 6); break;
        case 0xB1: state_[Reg8::C] = res(state_[Reg8::C], 6); break;
        case 0xB2: state_[Reg8::D] = res(state_[Reg8::D], 6); break;
        case 0xB3: state_[Reg8::E] = res(state_[Reg8::E], 6); break;
        case 0xB4: state_[Reg8::H] = res(state_[Reg8::H], 6); break;
        case 0xB5: state_[Reg8::L] = res(state_[Reg8::L], 6); break;
        case 0xB6: writeAtAddr(state_[Reg16::HL], res(readAtAddr(state_[Reg16::HL]), 6)); break;
        case 0xB7: state_[Reg8::A] = res(state_[Reg8::A], 6); break;
        case 0xB8: state_[Reg8::B] = res(state_[Reg8::B], 7); break;
        case 0xB9: state_[Reg8::C] = res(state_[Reg8::C], 7); break;
        case 0xBA: state_[Reg8::D] = res(state_[Reg8::D], 7); break;
        case 0xBB: state_[Reg8::E] = res(state_[Reg8::E], 7); break;
        case 0xBC: state_[Reg8::H] = res(state_[Reg8::H], 7); break;
        case 0xBD: state_[Reg8::L] = res(state_[Reg8::L], 7); break;
        case 0xBE: writeAtAddr(state_[Reg16::HL], res(readAtAddr(state_[Reg16::HL]), 7)); break;
        case 0xBF: state_[Reg8::A] = res(state_[Reg8::A], 7); break;

        // ====== Bit set block (0xC0 - 0xFF) ======

        case 0xC0: state_[Reg8::B] = set(state_[Reg8::B], 0); break;
        case 0xC1: state_[Reg8::C] = set(state_[Reg8::C], 0); break;
        case 0xC2: state_[Reg8::D] = set(state_[Reg8::D], 0); break;
        case 0xC3: state_[Reg8::E] = set(state_[Reg8::E], 0); break;
        case 0xC4: state_[Reg8::H] = set(state_[Reg8::H], 0); break;
        case 0xC5: state_[Reg8::L] = set(state_[Reg8::L], 0); break;
        case 0xC6: writeAtAddr(state_[Reg16::HL], set(readAtAddr(state_[Reg16::HL]), 0)); break;
        case 0xC7: state_[Reg8::A] = set(state_[Reg8::A], 0); break;
        case 0xC8: state_[Reg8::B] = set(state_[Reg8::B], 1); break;
        case 0xC9: state_[Reg8::C] = set(state_[Reg8::C], 1); break;
        case 0xCA: state_[Reg8::D] = set(state_[Reg8::D], 1); break;
        case 0xCB: state_[Reg8::E] = set(state_[Reg8::E], 1); break;
        case 0xCC: state_[Reg8::H] = set(state_[Reg8::H], 1); break;
        case 0xCD: state_[Reg8::L] = set(state_[Reg8::L], 1); break;
        case 0xCE: writeAtAddr(state_[Reg16::HL], set(readAtAddr(state_[Reg16::HL]), 1)); break;
        case 0xCF: state_[Reg8::A] = set(state_[Reg8::A], 1); break;

        case 0xD0: state_[Reg8::B] = set(state_[Reg8::B], 2); break;
        case 0xD1: state_[Reg8::C] = set(state_[Reg8::C], 2); break;
        case 0xD2: state_[Reg8::D] = set(state_[Reg8::D], 2); break;
        case 0xD3: state_[Reg8::E] = set(state_[Reg8::E], 2); break;
        case 0xD4: state_[Reg8::H] = set(state_[Reg8::H], 2); break;
        case 0xD5: state_[Reg8::L] = set(state_[Reg8::L], 2); break;
        case 0xD6: writeAtAddr(state_[Reg16::HL], set(readAtAddr(state_[Reg16::HL]), 2)); break;
        case 0xD7: state_[Reg8::A] = set(state_[Reg8::A], 2); break;
        case 0xD8: state_[Reg8::B] = set(state_[Reg8::B], 3); break;
        case 0xD9: state_[Reg8::C] = set(state_[Reg8::C], 3); break;
        case 0xDA: state_[Reg8::D] = set(state_[Reg8::D], 3); break;
        case 0xDB: state_[Reg8::E] = set(state_[Reg8::E], 3); break;
        case 0xDC: state_[Reg8::H] = set(state_[Reg8::H], 3); break;
        case 0xDD: state_[Reg8::L] = set(state_[Reg8::L], 3); break;
        case 0xDE: writeAtAddr(state_[Reg16::HL], set(readAtAddr(state_[Reg16::HL]), 3)); break;
        case 0xDF: state_[Reg8::A] = set(state_[Reg8::A], 3); break;
        
        case 0xE0: state_[Reg8::B] = set(state_[Reg8::B], 4); break;
        case 0xE1: state_[Reg8::C] = set(state_[Reg8::C], 4); break;
        case 0xE2: state_[Reg8::D] = set(state_[Reg8::D], 4); break;
        case 0xE3: state_[Reg8::E] = set(state_[Reg8::E], 4); break;
        case 0xE4: state_[Reg8::H] = set(state_[Reg8::H], 4); break;
        case 0xE5: state_[Reg8::L] = set(state_[Reg8::L], 4); break;
        case 0xE6: writeAtAddr(state_[Reg16::HL], set(readAtAddr(state_[Reg16::HL]), 4)); break;
        case 0xE7: state_[Reg8::A] = set(state_[Reg8::A], 4); break;
        case 0xE8: state_[Reg8::B] = set(state_[Reg8::B], 5); break;
        case 0xE9: state_[Reg8::C] = set(state_[Reg8::C], 5); break;
        case 0xEA: state_[Reg8::D] = set(state_[Reg8::D], 5); break;
        case 0xEB: state_[Reg8::E] = set(state_[Reg8::E], 5); break;
        case 0xEC: state_[Reg8::H] = set(state_[Reg8::H], 5); break;
        case 0xED: state_[Reg8::L] = set(state_[Reg8::L], 5); break;
        case 0xEE: writeAtAddr(state_[Reg16::HL], set(readAtAddr(state_[Reg16::HL]), 5)); break;
        case 0xEF: state_[Reg8::A] = set(state_[Reg8::A], 5); break;

        case 0xF0: state_[Reg8::B] = set(state_[Reg8::B], 6); break;
        case 0xF1: state_[Reg8::C] = set(state_[Reg8::C], 6); break;
        case 0xF2: state_[Reg8::D] = set(state_[Reg8::D], 6); break;
        case 0xF3: state_[Reg8::E] = set(state_[Reg8::E], 6); break;
        case 0xF4: state_[Reg8::H] = set(state_[Reg8::H], 6); break;
        case 0xF5: state_[Reg8::L] = set(state_[Reg8::L], 6); break;
        case 0xF6: writeAtAddr(state_[Reg16::HL], set(readAtAddr(state_[Reg16::HL]), 6)); break;
        case 0xF7: state_[Reg8::A] = set(state_[Reg8::A], 6); break;
        case 0xF8: state_[Reg8::B] = set(state_[Reg8::B], 7); break;
        case 0xF9: state_[Reg8::C] = set(state_[Reg8::C], 7); break;
        case 0xFA: state_[Reg8::D] = set(state_[Reg8::D], 7); break;
        case 0xFB: state_[Reg8::E] = set(state_[Reg8::E], 7); break;
        case 0xFC: state_[Reg8::H] = set(state_[Reg8::H], 7); break;
        case 0xFD: state_[Reg8::L] = set(state_[Reg8::L], 7); break;
        case 0xFE: writeAtAddr(state_[Reg16::HL], set(readAtAddr(state_[Reg16::HL]), 7)); break;
        case 0xFF: state_[Reg8::A] = set(state_[Reg8::A], 7); break;
    }
}

}
