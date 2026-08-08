/*basically like a skin in a game, changes only it's appearence not da way it does smth*/
/*
E basically just Emitter, like emitter generate this instruction
okay maybe i need to use this kind of documentation is da code because i can see it
a lot better without just one lining every comment
and ye you can also make it even simplier, skin.h with _E for easy instructions
and instructions.h with _A are for strong bad instructions
_E(macros) skin, _A(instruction) strong bad instruction

how to add:
MACRO: add E_xxx to x64_skin.h
FULL: add A_xxx to x64_instructions.h


*/
#pragma once
#include "x64_core.h"

//64bit registers
#define rax 0
#define rcx 1
#define rdx 2
#define rbx 3
#define rsp 4
#define rbp 5
#define rsi 6
#define rdi 7
#define r8  8
#define r9  9
#define r10 10
#define r11 11
#define r12 12
#define r13 13
#define r14 14
#define r15 15

//32BIT
#define eax   0
#define ecx   1
#define edx   2
#define ebx   3
#define esp   4
#define ebp   5
#define esi   6
#define edi   7
#define r8d   8
#define r9d   9
#define r10d 10
#define r11d 11
#define r12d 12
#define r13d 13
#define r14d 14
#define r15d 15

//16BIT registers
#define ax    0
#define cx    1
#define dx    2
#define bx    3
#define sp    4
#define bp    5
#define si    6
#define di    7

//8BIT registers
#define al    0
#define cl    1
#define dl    2
#define bl    3
#define ah    8
#define ch    5
#define dh    6
#define bh    7

#define E_push(reg) do { \
    if ((reg) >= 8) E_emit(&e, 0x41); \
    E_emit(&e, 0x50 + ((reg) & 7)); \
} while(0)

#define E_pop(reg) do { \
    if ((reg) >= 8) E_emit(&e, 0x41); \
    E_emit(&e, 0x58 + ((reg) & 7)); \
} while(0)

#define E_ret()     E_emit(&e, 0xC3)
#define E_nop()     E_emit(&e, 0x90)
#define E_int3()    E_emit(&e, 0xCC)

#define E_mov_rr(emitter, dst, src) do { \
    uint8_t rex = 0x48; \
    if ((src) >= 8) rex |= 0x04; \
    if ((dst) >= 8) rex |= 0x01; \
    E_emit(emitter, rex); \
    E_emit(emitter, 0x89); \
    E_emit(emitter, 0xC0 | (((src) & 7) << 3) | ((dst) & 7)); \
} while(0)

/*#define E_mov_ri(reg, imm) do { \
    uint8_t rex = 0x48; \
    if ((reg) >= 8) rex |= 0x01; \
    E_emit(&e, rex); \
    E_emit(&e, 0xB8 + ((reg) & 7)); \
    E_qword(&e, (uint64_t)(imm)); \
} while(0)
*/
//well.h
/*#define E_mov_ri(reg, imm) do { \
    if ((imm) == 0) { \
        uint8_t rex = 0x48; \
        if ((reg) >= 8) rex |= 0x04 | 0x01; /* REX.R + REX.B */ //\
                if ((reg) >= 8) { \
                    E_emit(&e, rex); \
                } else { \
                    E_emit(&e, rex); /* REX.W for 64-bit */ \
                } \
                E_emit(&e, 0x31); /* XOR r/m64, r64 */ \
                E_emit(&e, 0xC0 | (((reg)&7)<<3) | ((reg)&7)); \
            } else { \
                uint8_t rex = 0x48; if ((reg) >= 8) rex |= 0x01; \
                E_emit(&e, rex); E_emit(&e, 0xB8 + ((reg) & 7)); E_qword(&e, (uint64_t)(imm)); \
            } \
        } while(0)*/
#define E_mov_ri(reg, imm) do { \
    if ((imm) == 0) { \
        uint8_t rex = 0x48; /* REX.W */ \
        if ((reg) >= 8) rex |= 0x01; /* only REX.B for addition to rm */ \
        E_emit(&e, rex); \
        E_emit(&e, 0x31); \
        E_emit(&e, 0xC0 | ((reg) & 7) | (((reg) & 7) << 3)); /* same reg and rm */ \
    } else { \
        uint8_t rex = 0x48; if ((reg) >= 8) rex |= 0x01; \
        E_emit(&e, rex); E_emit(&e, 0xB8 + ((reg) & 7)); E_qword(&e, (uint64_t)(imm)); \
    } \
} while(0)

//ARITHMETIC
#define E_add_rr(dst, src) do { \
    uint8_t rex = 0x48; \
    if ((src) >= 8) rex |= 0x04; \
    if ((dst) >= 8) rex |= 0x01; \
    E_emit(&e, rex); \
    E_emit(&e, 0x01); \
    E_emit(&e, 0xC0 | (((src) & 7) << 3) | ((dst) & 7)); \
} while(0)
#define E_add_ri(reg, imm) do { \
    if ((imm) == 1) { \
        E_inc(reg); \
    } else if ((imm) >= -128 && (imm) <= 127) { \
        uint8_t rex = 0x48; if ((reg) >= 8) rex |= 0x01; \
        E_emit(&e, rex); E_emit(&e, 0x83); \
        E_emit(&e, 0xC0 | ((reg) & 7)); \
        E_emit(&e, (uint8_t)(imm)); \
    } else { \
        uint8_t rex = 0x48; if ((reg) >= 8) rex |= 0x01; \
        E_emit(&e, rex); E_emit(&e, 0x81); \
        E_emit(&e, 0xC0 | ((reg) & 7)); \
        E_dword(&e, (uint32_t)(imm)); \
    } \
} while(0)
#define E_sub_rr(dst, src) do { \
    uint8_t rex = 0x48; \
    if ((src) >= 8) rex |= 0x04; \
    if ((dst) >= 8) rex |= 0x01; \
    E_emit(&e, rex); \
    E_emit(&e, 0x29); \
    E_emit(&e, 0xC0 | (((src) & 7) << 3) | ((dst) & 7)); \
} while(0)
#define E_sub_ri(reg, imm) do { \
    if ((imm) == 1) { \
        E_dec(reg); \
    } else if ((imm) >= -128 && (imm) <= 127) { \
        uint8_t rex = 0x48; if ((reg) >= 8) rex |= 0x01; \
        E_emit(&e, rex); E_emit(&e, 0x83); \
        E_emit(&e, 0xE8 | ((reg) & 7)); \
        E_emit(&e, (uint8_t)(imm)); \
    } else { \
        uint8_t rex = 0x48; if ((reg) >= 8) rex |= 0x01; \
        E_emit(&e, rex); E_emit(&e, 0x81); \
        E_emit(&e, 0xE8 | ((reg) & 7)); \
        E_dword(&e, (uint32_t)(imm)); \
    } \
} while(0)

#define E_xor_rr(dst, src) do { \
    uint8_t rex = 0x48; \
    if ((src) >= 8) rex |= 0x04; \
    if ((dst) >= 8) rex |= 0x01; \
    E_emit(&e, rex); \
    E_emit(&e, 0x31); \
    E_emit(&e, 0xC0 | (((src) & 7) << 3) | ((dst) & 7)); \
} while(0)

#define E_cmp_rr(r1, r2) do { \
    uint8_t rex = 0x48; \
    if ((r2) >= 8) rex |= 0x04; \
    if ((r1) >= 8) rex |= 0x01; \
    E_emit(&e, rex); \
    E_emit(&e, 0x39); \
    E_emit(&e, 0xC0 | (((r2) & 7) << 3) | ((r1) & 7)); \
} while(0)
#define E_cmp_ri(reg, imm) do { \
    if ((imm) == 0) { \
        uint8_t rex = 0x48; \
        if ((reg) >= 8) rex |= 0x04 | 0x01; /* REX.R + REX.B for both src and dst */ \
        E_emit(&e, rex); E_emit(&e, 0x85); \
        E_emit(&e, 0xC0 | (((reg)&7)<<3) | ((reg)&7)); \
    } else if ((imm) >= -128 && (imm) <= 127) { \
        uint8_t rex = 0x48; if ((reg) >= 8) rex |= 0x01; \
        E_emit(&e, rex); E_emit(&e, 0x83); \
        E_emit(&e, 0xF8 | ((reg) & 7)); \
        E_emit(&e, (uint8_t)(imm)); \
    } else { \
        uint8_t rex = 0x48; if ((reg) >= 8) rex |= 0x01; \
        E_emit(&e, rex); E_emit(&e, 0x81); \
        E_emit(&e, 0xF8 | ((reg) & 7)); \
        E_dword(&e, (uint32_t)(imm)); \
    } \
} while(0)
#define E_inc(reg) do { \
    uint8_t rex = 0x48; \
    if ((reg) >= 8) rex |= 0x01; \
    E_emit(&e, rex); \
    E_emit(&e, 0xFF); \
    E_emit(&e, 0xC0 | ((reg) & 7)); \
} while(0)

#define E_dec(reg) do { \
    uint8_t rex = 0x48; \
    if ((reg) >= 8) rex |= 0x01; \
    E_emit(&e, rex); \
    E_emit(&e, 0xFF); \
    E_emit(&e, 0xC8 | ((reg) & 7)); \
} while(0)

//CALL/JMP
#define E_call(reg) do { \
    if ((reg) >= 8) E_emit(&e, 0x41); \
    E_emit(&e, 0xFF); \
    E_emit(&e, 0xD0 | ((reg) & 7)); \
} while(0)

#define E_jmp_reg(reg) do { \
    if ((reg) >= 8) E_emit(&e, 0x41); \
    E_emit(&e, 0xFF); \
    E_emit(&e, 0xE0 | ((reg) & 7)); \
} while(0)

#define E_shl_ri(reg, imm) do { \
    uint8_t rex = 0x48; \
    if ((reg) >= 8) rex |= 0x01; \
    E_emit(&e, rex); E_emit(&e, 0xC1); \
    E_emit(&e, 0xE0 | ((reg) & 7)); \
    E_emit(&e, (uint8_t)(imm)); \
} while(0)
#define E_shr_ri(reg, imm) do { \
    uint8_t rex = 0x48; \
    if ((reg) >= 8) rex |= 0x01; \
    E_emit(&e, rex); E_emit(&e, 0xC1); \
    E_emit(&e, 0xE8 | ((reg) & 7)); \
    E_emit(&e, (uint8_t)(imm)); \
} while(0)

#define E_not_r(reg) do { \
    uint8_t rex = 0x48; \
    if ((reg) >= 8) rex |= 0x01; \
    E_emit(&e, rex); E_emit(&e, 0xF7); \
    E_emit(&e, 0xD0 | ((reg) & 7)); \
} while(0)
#define E_neg_r(reg) do { \
    uint8_t rex = 0x48; \
    if ((reg) >= 8) rex |= 0x01; \
    E_emit(&e, rex); E_emit(&e, 0xF7); \
    E_emit(&e, 0xD8 | ((reg) & 7)); \
} while(0)

//SSE/AVX
/*#define E_vxorps_ymm(dst, src1, src2) do { \
    E_emit(&e, 0xC5); \
    E_emit(&e, ((~((src1)&0xF))<<3) | 0xFC); \
    E_emit(&e, 0x57); \
    E_emit(&e, 0xC0 | (((dst)&7)<<3) | ((src2)&7)); \
} while(0)*/
#define E_vxorps_ymm(dst, src1, src2) do { \
    /* 3-byte VEX: C4 + R | X | B | M_MMMM + W + vvvv + L + pp */ \
    uint8_t r_bit = ((dst) >= 8) ? 0 : 0x80; \
    uint8_t x_bit = 0x40; /* not used well uhh */ \
    uint8_t b_bit = ((src2) >= 8) ? 0 : 0x20; \
    uint8_t m_mmmm = 0x01; /* 00001 = 0F opcode map */ \
    uint8_t w_bit = 0; /* W=0 for 128-bit, for ymm doesn't matter */ \
    uint8_t vvvv = (~(src1)) & 0xF; \
    uint8_t l_bit = 0x04; /* L=1 for 256-bit YMM */ \
    uint8_t pp = 0x01; /* 66h prefix */ \
    E_emit(&e, 0xC4); \
    E_emit(&e, r_bit | x_bit | b_bit | m_mmmm); \
    E_emit(&e, (vvvv << 3) | l_bit | pp); \
    E_emit(&e, 0x57); /* VXORPS opcode */ \
    E_emit(&e, 0xC0 | (((dst)&7)<<3) | ((src2)&7)); \
} while(0)

#define E_vaddps_ymm(dst, src1, src2) do { \
    E_emit(&e, 0xC5); \
    E_emit(&e, 0xFC); \
    E_emit(&e, 0x58); \
    E_emit(&e, 0xC0 | (((dst)&7)<<3) | ((src2)&7)); \
} while(0)

#define E_vmulps_ymm(dst, src1, src2) do { \
    E_emit(&e, 0xC5); \
    E_emit(&e, 0xFC); \
    E_emit(&e, 0x59); \
    E_emit(&e, 0xC0 | (((dst)&7)<<3) | ((src2)&7)); \
} while(0)

#define E_vmovdqu_ymm(dst, src) do { \
    E_emit(&e, 0xC5); \
    E_emit(&e, 0xFE); \
    E_emit(&e, 0x6F); \
    E_emit(&e, 0xC0 | (((dst)&7)<<3) | ((src)&7)); \
} while(0)

//F16C (float16)
#define E_vcvtph2ps(dst, src) do { \
    /*VEX.128.66.0F38.W0 13 /r */ \
    if ((dst) >= 16 || (src) >= 16) { \
        /* AVX-512: EVEX */ \
        E_emit(&e, 0x62); \
        E_emit(&e, 0xF2); \
        E_emit(&e, 0x6D); \
        E_emit(&e, 0x08); \
        E_emit(&e, 0x13); \
        E_emit(&e, 0xC0 | (((dst) & 15) << 3) | ((src) & 15)); \
    } else { \
        /* AVX: VEX.128.66.0F38.W0 13 /r */ \
        E_emit(&e, 0xC5); \
        E_emit(&e, 0x7A); \
        E_emit(&e, 0x13); \
        E_emit(&e, 0xC0 | (((dst) & 7) << 3) | ((src) & 7)); \
    } \
} while(0)

//F16C: single-precision (float32) → half-precision (float16)
//vcvtps2ph xmm1, ymm0, 0  (converts 8 float32 to 8 float16, saves in xmm1)
#define E_vcvtps2ph(dst, src, rounding) do { \
    /* VEX.128.66.0F3A.W0 1D /r ib */ \
    if ((dst) >= 16 || (src) >= 16) { \
        /*AVX-512: EVEX */ \
        E_emit(&e, 0x62); \
        E_emit(&e, 0xF2); \
        E_emit(&e, 0x3D); \
        E_emit(&e, 0x08); \
        E_emit(&e, 0x1D); \
        E_emit(&e, 0xC0 | (((src) & 15) << 3) | ((dst) & 15)); \
        E_emit(&e, (rounding) & 0xFF); \
    } else { \
        /* AVX: VEX.128.66.0F3A.W0 1D /r ib */ \
        E_emit(&e, 0xC5); \
        E_emit(&e, 0x7A); \
        E_emit(&e, 0x1D); \
        E_emit(&e, 0xC0 | (((src) & 7) << 3) | ((dst) & 7)); \
        E_emit(&e, (rounding) & 0xFF); \
    } \
} while(0)

//FMA (multiplying+well plussing)
//#define E_vfmadd231ps(dst, src1, src2) do { \
    uint8_t vex_m_w = 0x42; /* M-MMMMM = 00010, W = 0 */ \
    uint8_t vex_r_v_l_p = 0x7D; /* L=1 (256-bit), pp=01 (66h) */ \
    /*invert bits of registers VEX, cuz well they're inverted*/ \
    uint8_t r_bit = ((dst) >= 8) ? 0 : 0x80; \
    uint8_t x_bit = ((src2) >= 8) ? 0 : 0x40; \
    uint8_t b_bit = 0x20; \
    uint8_t vvvv = (~(src1)) & 0xF; \
    E_emit(&e, 0xC4); \
    E_emit(&e, r_bit | x_bit | b_bit | vex_m_w); \
    E_emit(&e, (vvvv << 3) | (1 << 2) | 0x01); \
    E_emit(&e, 0xB8); \
    E_emit(&e, 0xC0 | (((dst) & 7) << 3) | ((src2) & 7)); \
} while(0)*/
#define E_vfmadd231ps(dst, src1, src2) do { \
    uint8_t r_bit = ((dst) >= 8) ? 0 : 0x80;   /* R for dst in reg field */ \
    uint8_t x_bit = 0x40;                       /* X isn't used (no SIB) */ \
    uint8_t b_bit = ((src2) >= 8) ? 0 : 0x20;  /* B for src2 in rm field */ \
    uint8_t m_mmmm = 0x01; /* 00001 = 0F opcode map */ \
    uint8_t w_bit = 0; /* W=0 */ \
    uint8_t vvvv = (~(src1)) & 0xF; \
    uint8_t l_bit = 0x04; /* L=1 for 256-bit YMM */ \
    uint8_t pp = 0x01; /* 66h prefix */ \
    E_emit(&e, 0xC4); \
    E_emit(&e, r_bit | x_bit | b_bit | m_mmmm); \
    E_emit(&e, (vvvv << 3) | l_bit | pp); \
    E_emit(&e, 0xB8); /* VFMADD231PS opcode (0x0F38 0xB8) */ \
    E_emit(&e, 0xC0 | (((dst)&7)<<3) | ((src2)&7)); \
} while(0)

//sheesh for registers without captain rex
#define E_mov_r8_r8(dst, src) do { \
    /* if using AH/CH/DH/BH then don't use REX! */ \
    if ((dst) >= 8 || (src) >= 8) { /* high-byte registers */ \
        /* use opcode without REX */ \
        E_emit(&e, 0x88); /* MOV r/m8, r8 (non REX) */ \
        E_emit(&e, 0xC0 | (((src)&7)<<3) | ((dst)&7)); \
    } else { \
        /* normal 8bit registers (AL, CL, DL, BL) */ \
        E_emit(&e, 0x88); /* non REX */ \
        E_emit(&e, 0xC0 | (((src)&7)<<3) | ((dst)&7)); \
    } \
} while(0)

//bitted for SHA-256 or smth
#define E_rol_ri(reg, imm) do { \
    uint8_t rex = 0x48; \
    if ((reg) >= 8) rex |= 0x01; \
    E_emit(&e, rex); E_emit(&e, 0xC1); \
    E_emit(&e, 0xC0 | ((reg) & 7)); \
    E_emit(&e, (uint8_t)(imm)); \
} while(0)
#define E_ror_ri(reg, imm) do { \
    uint8_t rex = 0x48; \
    if ((reg) >= 8) rex |= 0x01; \
    E_emit(&e, rex); E_emit(&e, 0xC1); \
    E_emit(&e, 0xC8 | ((reg) & 7)); \
    E_emit(&e, (uint8_t)(imm)); \
} while(0)
#define E_and_rr(dst, src) do { \
    uint8_t rex = 0x48; \
    if ((src) >= 8) rex |= 0x04; \
    if ((dst) >= 8) rex |= 0x01; \
    E_emit(&e, rex); E_emit(&e, 0x21); \
    E_emit(&e, 0xC0 | (((src)&7)<<3) | ((dst)&7)); \
} while(0)
#define E_or_rr(dst, src) do { \
    uint8_t rex = 0x48; \
    if ((src) >= 8) rex |= 0x04; \
    if ((dst) >= 8) rex |= 0x01; \
    E_emit(&e, rex); E_emit(&e, 0x09); \
    E_emit(&e, 0xC0 | (((src)&7)<<3) | ((dst)&7)); \
} while(0)

// YMM/XMX
#define ymm0  0
#define ymm1  1
#define ymm2  2
#define ymm3  3
#define ymm4  4
#define ymm5  5
#define ymm6  6
#define ymm7  7
#define ymm8  8
#define ymm9  9
#define ymm10 10
#define ymm11 11
#define ymm12 12
#define ymm13 13
#define ymm14 14
#define ymm15 15

#define xmm0  0
#define xmm1  1
#define xmm2  2
#define xmm3  3
#define xmm4  4
#define xmm5  5
#define xmm6  6
#define xmm7  7