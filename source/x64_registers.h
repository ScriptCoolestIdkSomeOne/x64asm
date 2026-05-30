#pragma once
#include <stdint.h>

enum class Reg64 : uint8_t {
    RAX = 0, RCX = 1, RDX = 2, RBX = 3,
    RSP = 4, RBP = 5, RSI = 6, RDI = 7,
    R8 = 8, R9 = 9, R10 = 10, R11 = 11,
    R12 = 12, R13 = 13, R14 = 14, R15 = 15
};
enum class Reg32 : uint8_t {
    EAX = 0, ECX = 1, EDX = 2, EBX = 3,
    ESP = 4, EBP = 5, ESI = 6, EDI = 7,
    R8D = 8, R9D = 9, R10D = 10, R11D = 11,
    R12D = 12, R13D = 13, R14D = 14, R15D = 15
};
enum class Reg8 : uint8_t {
    AL = 0, CL = 1, DL = 2, BL = 3,
    SPL = 4, BPL = 5, SIL = 6, DIL = 7,
    R8B = 8, R9B = 9, R10B = 10, R11B = 11,
    R12B = 12, R13B = 13, R14B = 14, R15B = 15
};
constexpr uint8_t REX = 0x40;
constexpr uint8_t REX_W = 0x48;
constexpr uint8_t REX_R = 0x44;
constexpr uint8_t REX_X = 0x42;
constexpr uint8_t REX_B = 0x41;
inline uint8_t reg_index(Reg64 r) { return static_cast<uint8_t>(r); }
inline uint8_t reg_index(Reg32 r) { return static_cast<uint8_t>(r); }
inline uint8_t reg_index(Reg8 r) { return static_cast<uint8_t>(r) & 0x7; }
inline bool reg_needs_rex(Reg64 r) { return reg_index(r) >= 8; }
inline bool reg_needs_rex(Reg32 r) { return reg_index(r) >= 8; }
inline bool reg_needs_rex(Reg8 r) { return static_cast<uint8_t>(r) >= 8; }