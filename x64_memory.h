/*
* memory handling stuff, backend and stuff
*/
#pragma once
#include <stdint.h>
#include "x64_registers.h"

typedef struct {
    Reg64 base;
    Reg64 index;
    uint8_t scale;
    int32_t disp;
    int has_index;
    int use_rip;
    void* abs_addr;
} Mem;
static Mem Mem_ptr(Reg64 b, int32_t d) {
    Mem m = { b, REG_RAX, 1, d, 0, 0, NULL };
    return m;
}
static Mem Mem_ptr_idx(Reg64 b, Reg64 idx, uint8_t sc, int32_t d) {
    Mem m = { b, idx, sc, d, 1, 0, NULL };
    return m;
}
static Mem Mem_abs(void* addr) {
    Mem m = { REG_RAX, REG_RAX, 1, 0, 0, 1, addr };
    return m;
}