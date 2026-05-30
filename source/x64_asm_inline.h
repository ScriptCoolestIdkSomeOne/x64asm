//might not work properly 
#pragma once
#include "x64_asm.h"

#define __asm_block \
    for (struct { X64Assembler a_; bool run_; } __s = { {}, true }; \
         __s.run_; \
         __s.run_ = false, __s.a_.run())

#define __asm_ret(var) \
    for (struct { X64Assembler a_; bool run_; } __s = { {}, true }; \
         __s.run_; \
         __s.run_ = false, var = __s.a_.run_ret<decltype(var)>())

#define MOV(dst, src) __s.a_.mov(dst, src)
#define ADD(dst, src) __s.a_.add(dst, src)
#define SUB(dst, src) __s.a_.sub(dst, src)
#define RET()         __s.a_.ret()
#define NOP()         __s.a_.nop()
#define INT3()        __s.a_.int3()
#define XOR(dst, src) __s.a_.xor_(dst, src)
#define CMP(a, b)     __s.a_.cmp(a, b)
#define PUSH(reg)     __s.a_.push(reg)
#define POP(reg)      __s.a_.pop(reg)
#define INC(reg)      __s.a_.inc(reg)
#define DEC(reg)      __s.a_.dec(reg)
