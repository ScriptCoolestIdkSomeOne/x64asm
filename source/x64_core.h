#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include "x64_registers.h"

constexpr uint8_t OP_RET  = 0xC3;
constexpr uint8_t OP_NOP  = 0x90;
constexpr uint8_t OP_INT3 = 0xCC;

struct Arena {
    uint8_t* base;
    
    void init() {
        base = (uint8_t*)VirtualAlloc(NULL, 4096, 
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    }
    
    uint8_t* get_base() const { return base; }
    
    /*void reset() {
    }*/
    
    void free_all() {
        if (base) {
            VirtualFree(base, 0, MEM_RELEASE);
            base = NULL;
        }
    }
};

inline Arena& g_arena() {
    static Arena arena;
    static bool initialized = ([]() { arena.init(); return true; })();
    return arena;
}

class X64Emitter {
protected:
    uint8_t* start;
    uint8_t* cur;

public:
    X64Emitter() {
        start = g_arena().get_base();
        cur = start;
    }
    
    void begin() { cur = start; }
    
    void emit(uint8_t b) { *cur++ = b; }
    
        void emit_dword(uint32_t d) {
        *(volatile uint32_t*)cur = d; //for the exact instructions, example: mov dword ptr [rcx], eax
        cur += 4;
    }
    
    void emit_qword(uint64_t q) {
        *(volatile uint64_t*)cur = q; //for the exact instructions, example: mov qword ptr [rcx], rax
        cur += 8;
    }
    
    size_t current_pos() const { return (size_t)(cur - start); }
    
    void set_byte(size_t pos, uint8_t val) { start[pos] = val; }
    
    void set_dword(size_t pos, uint32_t val) {
        *(volatile uint32_t*)(start + pos) = val;
    }
	
    void emit_rex(bool w, bool r, bool x, bool b) {
        emit(0x40 | (w << 3) | (r << 2) | (x << 1) | b);
    }
    
    void emit_modrm(uint8_t mod, uint8_t reg, uint8_t rm) {
        emit(((mod & 0x3) << 6) | ((reg & 0x7) << 3) | (rm & 0x7));
    }

    void emit_sib(uint8_t scale, uint8_t index, uint8_t base) {
        emit(((scale & 0x3) << 6) | ((index & 0x7) << 3) | (base & 0x7));
    }

    void flush_icache() {
        _mm_mfence();
        _mm_lfence();
    }

    void run() {
        if (cur == start) return;
        if (cur[-1] != OP_RET) emit(OP_RET);
        flush_icache();
        ((void(*)())start)();
    }
    
    template<typename RetT = int>
    RetT run_ret() {
        if (cur == start) return RetT();
        if (cur[-1] != OP_RET) emit(OP_RET);
        flush_icache();
        return ((RetT(*)())start)();
    }
    
    void reset() { cur = start; }
    
    static void cleanup() {
        g_arena().free_all();
    }
    
    ~X64Emitter() {
        //use X64Emitter::cleanup()
    }
};//da