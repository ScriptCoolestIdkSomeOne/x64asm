#pragma once
#include <stdint.h>
#include "x64_registers.h"

struct Mem {
    Reg64 base = Reg64::RAX;
    Reg64 index = Reg64::RAX;
    uint8_t scale = 1;
    int32_t disp = 0;
    bool has_index = false;
    bool use_rip = false;
    void* abs_addr = nullptr;
    static Mem ptr(Reg64 b, int32_t d = 0) {
        Mem m;
        m.base = b;
        m.disp = d;
        return m;
    }
    static Mem ptr(Reg64 b, Reg64 idx, uint8_t sc, int32_t d = 0) {
        Mem m;
        m.base = b;
        m.index = idx;
        m.scale = sc;
        m.disp = d;
        m.has_index = true;
        return m;
    }
    static Mem abs(void* addr) {
        Mem m;
        m.use_rip = true;
        m.abs_addr = addr;
        return m;
    }
};