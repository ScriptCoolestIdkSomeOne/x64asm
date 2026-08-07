/*
* strong bad instructions and it looks nice and cool etc etc
* basically handles da instructions as you've already seen in da name of da file
* basically backend, don't touch, buuut if you want to you can
*/

#pragma once
#include "x64_core.h"
#include "x64_memory.h"
#include "x64_labels.h"

typedef struct {
    X64Emitter base;
    X64Labels labels;
} X64Assembler;

static void A_init(X64Assembler* a) {
    E_init(&a->base);
    L_init(&a->labels);
}

static void A_mov_r_imm(X64Assembler* a, Reg64 dst, uint64_t imm) {
    uint8_t rex = REX_W;
    if (dst >= 8) rex |= REX_B;
    E_emit(&a->base, rex);
    E_emit(&a->base, 0xB8 + (dst & 7));
    E_qword(&a->base, imm);
}

static void A_mov_r_r(X64Assembler* a, Reg64 dst, Reg64 src) {
    uint8_t rex = REX_W;
    if (dst >= 8) rex |= REX_B;
    if (src >= 8) rex |= REX_R;
    E_emit(&a->base, rex);
    E_emit(&a->base, 0x89);
    E_modrm(&a->base, 3, src & 7, dst & 7);
}

static void A_add_r_imm(X64Assembler* a, Reg64 dst, int imm) {
    uint8_t rex = REX_W;
    if (dst >= 8) rex |= REX_B;
    E_emit(&a->base, rex);
    if (imm >= -128 && imm <= 127) {
        E_emit(&a->base, 0x83); E_modrm(&a->base, 3, 0, dst & 7); E_emit(&a->base, (uint8_t)imm);
    }
    else {
        E_emit(&a->base, 0x81); E_modrm(&a->base, 3, 0, dst & 7); E_dword(&a->base, (uint32_t)imm);
    }
}

static void A_add_r_r(X64Assembler* a, Reg64 dst, Reg64 src) {
    uint8_t rex = REX_W;
    if (dst >= 8) rex |= REX_B;
    if (src >= 8) rex |= REX_R;
    E_emit(&a->base, rex);
    E_emit(&a->base, 0x01);
    E_modrm(&a->base, 3, src & 7, dst & 7);
}

static void A_ret(X64Assembler* a) { E_emit(&a->base, OP_RET); }
static void A_nop(X64Assembler* a) { E_emit(&a->base, OP_NOP); }
static void A_int3(X64Assembler* a) { E_emit(&a->base, OP_INT3); }

static void A_xor_r_r(X64Assembler* a, Reg64 dst, Reg64 src) {
    uint8_t rex = REX_W;
    if (dst >= 8) rex |= REX_B;
    if (src >= 8) rex |= REX_R;
    E_emit(&a->base, rex);
    E_emit(&a->base, 0x31);
    E_modrm(&a->base, 3, src & 7, dst & 7);
}

static void A_cmp_r_r(X64Assembler* a, Reg64 r1, Reg64 r2) {
    uint8_t rex = REX_W;
    if (r1 >= 8) rex |= REX_B;
    if (r2 >= 8) rex |= REX_R;
    E_emit(&a->base, rex);
    E_emit(&a->base, 0x39);
    E_modrm(&a->base, 3, r2 & 7, r1 & 7);
}

static void A_push(X64Assembler* a, Reg64 r) {
    if (r >= 8) E_emit(&a->base, REX_B);
    E_emit(&a->base, 0x50 + (r & 7));
}

static void A_pop(X64Assembler* a, Reg64 r) {
    if (r >= 8) E_emit(&a->base, REX_B);
    E_emit(&a->base, 0x58 + (r & 7));
}

static void A_inc(X64Assembler* a, Reg64 r) {
    uint8_t rex = REX_W;
    if (r >= 8) rex |= REX_B;
    E_emit(&a->base, rex); E_emit(&a->base, 0xFF);
    E_modrm(&a->base, 3, 0, r & 7);
}

static void A_dec(X64Assembler* a, Reg64 r) {
    uint8_t rex = REX_W;
    if (r >= 8) rex |= REX_B;
    E_emit(&a->base, rex); E_emit(&a->base, 0xFF);
    E_modrm(&a->base, 3, 1, r & 7);
}

static void A_sub_r_imm(X64Assembler* a, Reg64 dst, int imm) {
    uint8_t rex = REX_W;
    if (dst >= 8) rex |= REX_B;
    E_emit(&a->base, rex);
    if (imm >= -128 && imm <= 127) {
        E_emit(&a->base, 0x83); E_modrm(&a->base, 3, 5, dst & 7); E_emit(&a->base, (uint8_t)imm);
    }
    else {
        E_emit(&a->base, 0x81); E_modrm(&a->base, 3, 5, dst & 7); E_dword(&a->base, (uint32_t)imm);
    }
}

static void A_sub_r_r(X64Assembler* a, Reg64 dst, Reg64 src) {
    uint8_t rex = REX_W;
    if (dst >= 8) rex |= REX_B;
    if (src >= 8) rex |= REX_R;
    E_emit(&a->base, rex); E_emit(&a->base, 0x29);
    E_modrm(&a->base, 3, src & 7, dst & 7);
}

static void A_jmp(X64Assembler* a, const char* label) {
    int32_t off = L_get(&a->labels, label);
    if (off >= 0) {
        int32_t d = off - ((int32_t)E_pos(&a->base) + 5);
        E_emit(&a->base, 0xE9); E_dword(&a->base, d);
    }
    else {
        L_add(&a->labels, label, E_pos(&a->base), E_pos(&a->base) + 5, 0);
        E_emit(&a->base, 0xE9); E_dword(&a->base, 0);
    }
}

static void A_bind(X64Assembler* a, const char* name) {
    L_bind(&a->labels, name, E_pos(&a->base));
}

static void A_call(X64Assembler* a, Reg64 r) {
    if (r >= 8) E_emit(&a->base, REX_B);
    E_emit(&a->base, 0xFF); E_modrm(&a->base, 3, 2, r & 7);
}

static void A_run(X64Assembler* a) {
    L_resolve(&a->labels, &a->base, E_pos(&a->base));
    E_run(&a->base);
    L_reset(&a->labels);
    E_begin(&a->base);
}

#define A_run_ret(a, type) E_run_ret(&(a)->base, type)