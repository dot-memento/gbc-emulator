#pragma once

#include <iomanip>
#include <iostream>
#include <type_traits>
#include <array>

#include "types.hpp"

namespace GbcEmulator {

enum class Reg16 { PC=0, AF=2, BC=4, DE=6, HL=8, SP=10 };
enum class Reg8  { F=2, A=3, C=4, B=5, E=6, D=7, L=8, H=9 };
enum class RegFlag : Byte { C=1<<4, H=1<<5, N=1<<6, Z=1<<7 };

class _Reg16
{
public:
    constexpr _Reg16(Byte& hi, Byte& lo) : _hi{hi}, _lo{lo} {}
    _Reg16(const _Reg16&) = delete;
    _Reg16& operator=(const _Reg16&) = delete;

    constexpr _Reg16& operator=(Word value) {
        _hi = static_cast<Byte>(value >> 8) & 0xFFu;
        _lo = value & 0xFFu;
        return *this;
    }

    constexpr operator Word() const {
        return (_hi << 8) | _lo;
    }

    constexpr Word operator++(int) {
        Word value = operator Word();
        operator+=(1);
        return value;
    }

    constexpr _Reg16& operator++() {
        return operator+=(1);
    }

    constexpr _Reg16& operator+=(Word rhs) {
        return operator=(operator Word() + rhs);
    }

    constexpr Word operator--(int) {
        Word value = operator Word();
        operator-=(1);
        return value;
    }

    constexpr _Reg16& operator--() {
        return operator-=(1);
    }

    constexpr _Reg16& operator-=(Word rhs) {
        return operator=(operator Word() - rhs);
    }

private:
    Byte& _hi;
    Byte& _lo;
};

class _RegFlag
{
public:
    constexpr _RegFlag(Byte& byte, Byte mask) : _byte{byte}, _mask{mask} {}
    _RegFlag(const _RegFlag&) = delete;
    _RegFlag& operator=(const _RegFlag&) = delete;

    constexpr _RegFlag& operator=(bool value) {
        _byte = (~_mask & _byte) | (value ? _mask : 0);
        return *this;
    }

    constexpr operator bool() const {
        return _byte & _mask;
    }

private:
    Byte& _byte;
    Byte _mask;
};

class CpuState
{
public:
    constexpr Byte operator[](Reg8 r) const {
        return _reg[static_cast<std::array<unsigned char, 12>::size_type>(r)];
    }
    
    constexpr Byte& operator[](Reg8 r) {
        return _reg[static_cast<std::array<unsigned char, 12>::size_type>(r)];
    }

    constexpr _Reg16 operator[](Reg16 rr) {
        auto rr_index = static_cast<std::array<unsigned char, 12>::size_type>(rr);
        return _Reg16{_reg[rr_index+1], _reg[rr_index]};
    }

    constexpr _RegFlag operator[](RegFlag f) {
        auto f_reg = static_cast<std::array<unsigned char, 12>::size_type>(Reg8::F);
        return _RegFlag {_reg[f_reg], static_cast<Byte>(f)};
    }

    bool ime, next_ime;
    bool paused;

    enum class Mode {
        Normal,
        Halted,
        Stopped,
    } mode;

    constexpr void reset() {
        paused = true;
        _reg.fill(0);
        next_ime = 0;
        ime = 0;
        mode = Mode::Normal;
    }

private:
    std::array<Byte, 12> _reg;
};

}  // namespace GbcEmulator
