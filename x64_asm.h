/*
* mainly main stuff for main stuff but this is not a core stuff but a main
* stuff
* like macrosses or basic instructions
* this wors pretty basic and somewhere weird
*/
#pragma once
#include "x64_instructions.h"
#include "memarehallok.h"

static const struct { const char* name; int reg; } reg_tbl[] = {
    {"rax",0},{"rcx",1},{"rdx",2},{"rbx",3},{"rsp",4},{"rbp",5},{"rsi",6},{"rdi",7},
    {"r8",8},{"r9",9},{"r10",10},{"r11",11},{"r12",12},{"r13",13},{"r14",14},{"r15",15},
};

static int P_find_reg(const char* s) {
    for (int i = 0; i < 16; i++) {
        const char* a = reg_tbl[i].name, * b = s;
        while (*a && *b && *a == *b) { a++; b++; }
        if (*a == 0 && *b == 0) return reg_tbl[i].reg;
    }
    return -1;
}

static int P_is_num(const char* s) {
    if (!*s) return 0;
    if (s[0] == '0' && s[1] == 'x') return 1;
    int i = (s[0] == '-') ? 1 : 0;
    return s[i] >= '0' && s[i] <= '9';
}

static uint64_t P_num(const char* s) {
    uint64_t n = 0;
    if (s[0] == '0' && s[1] == 'x') {
        for (int i = 2; s[i]; i++)
            n = n * 16 + (s[i] >= 'a' ? s[i] - 'a' + 10 : s[i] >= 'A' ? s[i] - 'A' + 10 : s[i] - '0');
    }
    else if (s[0] == '-') {
        int64_t v = (int64_t)P_num(s + 1);
        return (uint64_t)(-v);
    }
    else {
        for (int i = 0; s[i]; i++)
            n = n * 10 + s[i] - '0';
    }
    return n;
}

static uint32_t hash_str(const char* s) {
    uint32_t h = 5381;
    while (*s) h = ((h << 5) + h) + *s++;
    return h;
}

typedef struct { X64Assembler* a; } X64Parser;

/* BUGFIX: op was parsed after executing the instruction, so op contained
the previous instructions opcode and not the current one */
static void P_parse(X64Parser* p, const char* code) {
    char op[16], dst[16], src[16];
    const char* c = code;

    while (*c) {
        // Skip whitespace
        while (*c == ' ' || *c == '\t' || *c == '\n') c++;
        if (!*c) break;

        // Handle comments
        if (*c == ';') {
            while (*c && *c != '\n') c++;
            continue;
        }

        // Parse opcode
        int i = 0;
        while (*c && *c != ' ' && *c != '\t' && *c != '\n' && *c != ';' && i < 15) {
            op[i++] = *c++;
        }
        op[i] = 0;

        // Skip whitespace after opcode
        while (*c == ' ' || *c == '\t') c++;

        // Handle db pseudo-instruction
        if (hash_str(op) == hash_str("db")) {
            uint8_t raw[16]; int bc = 0;
            while (*c && *c != ';' && *c != '\n' && bc < 16) {
                while (*c == ' ' || *c == ',') c++;
                if (*c && *c != ';' && *c != '\n') {
                    raw[bc++] = (uint8_t)P_num(c);
                    while (*c && *c != ' ' && *c != ',' && *c != ';' && *c != '\n') c++;
                }
            }
            if (bc > 0) E_bytes(&p->a->base, raw, bc);
            continue;
        }

        // Parse destination operand
        i = 0;
        while (*c && *c != ',' && *c != ';' && *c != '\n' && i < 15) {
            dst[i++] = *c++;
        }
        dst[i] = 0;
        while (i > 0 && (dst[i - 1] == ' ' || dst[i - 1] == '\t')) {
            dst[--i] = 0;
        }

        // Parse source operand if comma present
        src[0] = 0;
        if (*c == ',') {
            c++;
            while (*c == ' ' || *c == '\t') c++;
            i = 0;
            while (*c && *c != ';' && *c != '\n' && i < 15) {
                src[i++] = *c++;
            }
            src[i] = 0;
            while (i > 0 && (src[i - 1] == ' ' || src[i - 1] == '\t')) {
                src[--i] = 0;
            }
        }

        // Execute instruction
        uint32_t h = hash_str(op);
        int dr = P_find_reg(dst);

        if (h == hash_str("mov")) {
            if (dr < 0) continue;
            if (src[0]) {
                int sr = P_find_reg(src);
                if (sr >= 0) A_mov_r_r(p->a, (Reg64)dr, (Reg64)sr);
                else if (P_is_num(src)) A_mov_r_imm(p->a, (Reg64)dr, P_num(src));
            }
        }
        else if (h == hash_str("add")) {
            if (dr < 0) continue;
            if (src[0]) {
                int sr = P_find_reg(src);
                if (sr >= 0) A_add_r_r(p->a, (Reg64)dr, (Reg64)sr);
                else if (P_is_num(src)) A_add_r_imm(p->a, (Reg64)dr, (int)P_num(src));
            }
        }
        else if (h == hash_str("ret")) A_ret(p->a);
        else if (h == hash_str("nop")) A_nop(p->a);
        else if (h == hash_str("int3")) A_int3(p->a);
        else if (h == hash_str("push") && dr >= 0) A_push(p->a, (Reg64)dr);
        else if (h == hash_str("pop") && dr >= 0) A_pop(p->a, (Reg64)dr);
        else if (h == hash_str("inc") && dr >= 0) A_inc(p->a, (Reg64)dr);
        else if (h == hash_str("dec") && dr >= 0) A_dec(p->a, (Reg64)dr);
        else if (h == hash_str("jmp")) A_jmp(p->a, dst);

        // Skip separator - ONLY skip ; and whitespace, NOT the next instruction
        if (*c == ';') c++;
        while (*c == ' ' || *c == '\t' || *c == '\n') c++;
    }
}
/*runs immediately
#define __asm(code) do { \
    X64Assembler __a; \
    X64Parser __p; \
    A_init(&__a); \
    __p.a = &__a; \
    P_parse(&__p, code); \
    JIT_FLUSH_CODE(); \
    A_run(&__a); \
} while(0)
*/

#define __asm(code, out_ptr) do { \
    if (!g_jit_mem.initialized) JitMemory_init(); \
    static X64Assembler __a; \
    static X64Parser __p; \
    static int __initialized = 0; \
    if (!__initialized) { \
        A_init(&__a); \
        __p.a = &__a; \
        __initialized = 1; \
    } \
    size_t __start = E_pos(&__a.base); \
    P_parse(&__p, code); \
    size_t __sz = E_pos(&__a.base) - __start; \
    DWORD __old; \
    VirtualProtect(__a.base.start + __start, __sz, PAGE_EXECUTE_READWRITE, &__old); \
    FlushInstructionCache(GetCurrentProcess(), __a.base.start + __start, __sz); \
    (out_ptr) = (void*)(__a.base.start + __start); \
} while(0)

#define __asm_get_code(asm) ((asm)->base.mem)
