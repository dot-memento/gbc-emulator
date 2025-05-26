#pragma once

#include <bitset>

#include "clock.hpp"
#include "cpu_state.hpp"

namespace GbcEmulator {

class MemoryManagmentUnit;

class Cpu {
public:
    explicit Cpu(MemoryManagmentUnit& bus) : bus_{bus} { reset(); }
    Cpu(const Cpu&) = delete;
    Cpu& operator=(const Cpu&) = delete;

    constexpr Clock& getClock() { return clock_; }

    constexpr const CpuState& getState() const { return state_; }
    constexpr CpuState createStateSnapshot() const { return state_; }
    constexpr void restoreStateSnapshot(const CpuState& state) { state_ = state; }

    void reset();
    void setPause(bool is_paused = true) { state_.paused = is_paused; }
    void stepTCycles(TCycleCount n);

    bool hasBreakpoint(Word address) const { return breakpoints_.test(address); }
    void setBreakpoint(Word address) { breakpoints_.set(address); }
    void clearBreakpoint(Word address) { breakpoints_.set(address, false); }
    void clearAllBreakpoints() { breakpoints_.reset(); }

private:
    Byte readAtAddr(Word address);
    void writeAtAddr(Word address, Byte value);
    void writeWordAtAddr(Word address, Word value);

    Byte readNextByte();
    Word readNextWord();

    void baseInstruction(Byte inst);

    void stop();
    void halt();

    void ei();
    void di();

    void loadAddSigned(_Reg16 regTo, Word value, Byte offset);

    void incAddr(_Reg16 addr);
    void decAddr(_Reg16 addr);
    void incWord(_Reg16 reg);
    void decWord(_Reg16 reg);
    void incByte(Byte& reg);
    void decByte(Byte& reg);

    void addWord(_Reg16 regTo, Word value);
    void addWordSigned(_Reg16 regTo, Byte value);
    void add(Byte& regTo, Byte value);
    void adc(Byte& regTo, Byte value);
    void sub(Byte& regTo, Byte value);
    void sbc(Byte& regTo, Byte value);
    void bitwiseAnd(Byte& regTo, Byte value);
    void bitwiseXor(Byte& regTo, Byte value);
    void bitwiseOr(Byte& regTo, Byte value);
    void cp(Byte regA, Byte regB);

    void rlca();
    void rla();
    void rrca();
    void rra();

    void daa();
    void cpl();
    void scf();
    void ccf();

    void jmpRel(Byte offset, bool condition = true);
    void jmp(Word address, bool condition = true);

    void push(Word value);
    void pop(_Reg16 regTo);

    void call(Word address, bool condition = true);
    void ret();
    void ret(bool condition);
    void reti();

    void prefixInstruction(Byte inst);

    Byte rlc(Byte value);
    Byte rrc(Byte value);

    Byte rl(Byte value);
    Byte rr(Byte value);

    Byte sla(Byte value);
    Byte sra(Byte value);

    Byte swap(Byte value);
    Byte srl(Byte value);

    void bit(Byte value, Byte bit);
    Byte res(Byte value, Byte bit);
    Byte set(Byte value, Byte bit);

    void undefinedInstruction();

    MemoryManagmentUnit& bus_;
    CpuState state_;

    Clock clock_;

    std::bitset<0x10000> breakpoints_;

    friend class GameBoyDebugger;
};

}  // namespace GbcEmulator
