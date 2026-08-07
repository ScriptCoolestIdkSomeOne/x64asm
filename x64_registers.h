/*
* oldest remaining still file
* basically just a text book i would say
*/

#pragma once
#include <stdint.h>

typedef enum {
    REG_RAX = 0, REG_RCX = 1, REG_RDX = 2, REG_RBX = 3,
    REG_RSP = 4, REG_RBP = 5, REG_RSI = 6, REG_RDI = 7,
    REG_R8 = 8, REG_R9 = 9, REG_R10 = 10, REG_R11 = 11,
    REG_R12 = 12, REG_R13 = 13, REG_R14 = 14, REG_R15 = 15
} Reg64;

typedef enum {
    REG32_EAX = 0, REG32_ECX = 1, REG32_EDX = 2, REG32_EBX = 3,
    REG32_ESP = 4, REG32_EBP = 5, REG32_ESI = 6, REG32_EDI = 7,
    REG32_R8D = 8, REG32_R9D = 9, REG32_R10D = 10, REG32_R11D = 11,
    REG32_R12D = 12, REG32_R13D = 13, REG32_R14D = 14, REG32_R15D = 15
} Reg32;

typedef enum {
    REG8_AL = 0, REG8_CL = 1, REG8_DL = 2, REG8_BL = 3,
    REG8_SPL = 4, REG8_BPL = 5, REG8_SIL = 6, REG8_DIL = 7,
    REG8_R8B = 8, REG8_R9B = 9, REG8_R10B = 10, REG8_R11B = 11,
    REG8_R12B = 12, REG8_R13B = 13, REG8_R14B = 14, REG8_R15B = 15
} Reg8;

typedef enum {
    XMM0 = 0, XMM1 = 1, XMM2 = 2, XMM3 = 3, XMM4 = 4, XMM5 = 5, XMM6 = 6, XMM7 = 7,
    XMM8 = 8, XMM9 = 9, XMM10 = 10, XMM11 = 11, XMM12 = 12, XMM13 = 13, XMM14 = 14, XMM15 = 15,
    YMM0 = 16, YMM1 = 17, YMM2 = 18, YMM3 = 19, YMM4 = 20, YMM5 = 21, YMM6 = 22, YMM7 = 23,
    YMM8 = 24, YMM9 = 25, YMM10 = 26, YMM11 = 27, YMM12 = 28, YMM13 = 29, YMM14 = 30, YMM15 = 31
} RegSIMD;

#define REX   0x40
#define REX_W 0x48
#define REX_R 0x44
#define REX_X 0x42
#define REX_B 0x41

static inline uint8_t reg_index(Reg64 r) { return (uint8_t)r; }
static inline uint8_t reg_index32(Reg32 r) { return (uint8_t)r; }
static inline uint8_t reg_index8(Reg8 r) { return (uint8_t)r & 0x7; }
static inline uint8_t reg_index_simd(RegSIMD r) { return (uint8_t)r & 0xF; }
static int reg_needs_rex64(Reg64 r) { return r >= 8; }
static int reg_needs_rex32(Reg32 r) { return r >= 8; }
static int reg_needs_rex8(Reg8 r) { return (uint8_t)r >= 8; }
static int reg_needs_rex_simd(RegSIMD r) { return (r & 0xF) >= 8; }