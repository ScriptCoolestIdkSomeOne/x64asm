#pragma once
#include "x64_instructions.h"
//fixed lines intead of std::string
struct StrBuf {
    char data[256];
    int len;
    void set(const char* s) {
        len = 0;
        while (*s && len < 255) data[len++] = *s++;
        data[len] = '\0';
    }
    bool eq(const char* s) {
        int i = 0;
        while (s[i] && data[i] && data[i] == s[i]) i++;
        return s[i] == data[i];
    }
};
//reg table
struct RegEntry {
    const char* name;
    Reg64 reg;
};

static const RegEntry reg_table[] = {
    {"rax", Reg64::RAX}, {"rcx", Reg64::RCX}, {"rdx", Reg64::RDX}, {"rbx", Reg64::RBX},
    {"rsp", Reg64::RSP}, {"rbp", Reg64::RBP}, {"rsi", Reg64::RSI}, {"rdi", Reg64::RDI},
    {"r8",  Reg64::R8},  {"r9",  Reg64::R9},  {"r10", Reg64::R10}, {"r11", Reg64::R11},
    {"r12", Reg64::R12}, {"r13", Reg64::R13}, {"r14", Reg64::R14}, {"r15", Reg64::R15},
};

class X64Parser {
    X64Assembler* asm_;

    //lining instead of ::map
    Optional<Reg64> find_reg(const char* s) {
        for (auto& entry : reg_table) {
            const char* a = entry.name;
            const char* b = s;
            while (*a && *b && *a == *b) { a++; b++; }
            if (*a == '\0' && *b == '\0') return Optional<Reg64>(entry.reg);
        }
        return Optional<Reg64>();
    }
    bool is_number(const char* s) {
        if (!s[0]) return false;
        if (s[0] == '0' && s[1] == 'x') return true;
        int i = 0;
        if (s[0] == '-') i = 1;
        return s[i] >= '0' && s[i] <= '9';
    }
    uint64_t parse_num(const char* s) {
        uint64_t n = 0;
        if (s[0] == '0' && s[1] == 'x') {
            for (int i = 2; s[i]; i++) {
                n = n * 16 + (s[i] >= 'a' ? s[i] - 'a' + 10 : s[i] >= 'A' ? s[i] - 'A' + 10 : s[i] - '0');
            }
        }
        else if (s[0] == '-') {
            int64_t v = parse_num(s + 1);// removed un - for uint64
            return (uint64_t)(-v);
        }
        else {
            for (int i = 0; s[i]; i++) n = n * 10 + s[i] - '0';
        }
        return n;
    }
public:
    X64Parser(X64Assembler* a) : asm_(a) {}
    static int my_strcmp(const char* a, const char* b) {
        while (*a && *b && *a == *b) { a++; b++; }
        return (unsigned char)*a - (unsigned char)*b;
    }
    void parse(const char* code) {
        char op[16], dst[16], src[16];
        const char* p = code;

        while (*p) {
            //fuck 
            while (*p == ' ' || *p == '\t' || *p == '\n') p++;
            if (!*p) break;
            //read opcode
            int i = 0;
            while (*p && *p != ' ' && *p != '\n' && *p != '\t' && i < 15) op[i++] = *p++;
            op[i] = '\0';
            //skip operands
            while (*p == ' ' || *p == '\t') p++;
            //read dst
            i = 0;
            while (*p && *p != ',' && *p != ';' && *p != '\n' && i < 15) dst[i++] = *p++;
            dst[i] = '\0';
            //read src
            if (*p == ',') {
                p++;
                while (*p == ' ') p++;
                i = 0;
                while (*p && *p != ';' && *p != '\n' && i < 15) src[i++] = *p++;
                src[i] = '\0';
            }
            else {
                src[0] = '\0';
            }
			//execute
            if (my_strcmp(op, "mov") == 0) {
                auto d = find_reg(dst);
                if (!d.has_value()) continue;
                if (src[0]) {
                    auto s = find_reg(src);
                    if (s.has_value()) asm_->mov(*d, *s);
                }
                else if (is_number(dst)) {
                    asm_->mov(*d, parse_num(dst));
                }
            }
            else if (my_strcmp(op, "add") == 0) {
                auto d = find_reg(dst);
                if (!d.has_value()) continue;
                if (src[0]) {
                    auto s = find_reg(src);
                    if (s.has_value()) asm_->add(*d, *s);
                }
                else if (is_number(dst)) {
                    asm_->add(*d, (int)parse_num(dst));
                }
            }
            else if (my_strcmp(op, "ret") == 0) asm_->ret();
            else if (my_strcmp(op, "nop") == 0) asm_->nop();
            else if (my_strcmp(op, "int3") == 0) asm_->int3();

            while (*p == ';') p++;
        }
    }
};

#define __asm(code) \
    do { \
        static X64Assembler __a; \
        static X64Parser __p(&__a); \
        __p.parse(code); \
        __a.run(); \
        __a.reset(); \
    } while(0)