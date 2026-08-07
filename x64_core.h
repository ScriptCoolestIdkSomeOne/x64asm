/*
* very very and veeery polised shit that i've ever done i think
* core of da headers, i've could've just make everyting in one header file but
* it's pretty confusing and weird and other stuff and i can't focus
* okay this file works as like basic instructions like backend that you don't need
* to touch
* gosh i really need to update da manual
*/
/*
change E_run_ret_int, E_run_ret_float and E_run_ret_void into one E_finalize
*/
#pragma once
#include <intrin.h>
#include <stdint.h>
#include <stddef.h>
#include "x64_registers.h"
#include "memarehallok.h"

#define WINAPI __stdcall


typedef void* HANDLE;
typedef HANDLE PVOID;
typedef unsigned long DWORD;
typedef DWORD* PDWORD;
typedef size_t SIZE_T;

//VirtualAlloc
#define MEM_COMMIT  0x00001000
#define MEM_RESERVE 0x00002000
#define MEM_RELEASE 0x00008000
#define PAGE_READWRITE       0x04
#define PAGE_EXECUTE_READ    0x20
#define PAGE_EXECUTE_READWRITE 0x40

__declspec(dllimport) PVOID __stdcall VirtualAlloc(PVOID lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect);
__declspec(dllimport) int   __stdcall VirtualFree(PVOID lpAddress, size_t dwSize, DWORD dwFreeType);
__declspec(dllimport) int   __stdcall VirtualProtect(PVOID lpAddress, size_t dwSize, DWORD flNewProtect, DWORD* lpflOldProtect);
__declspec(dllimport) int   __stdcall FlushInstructionCache(PVOID hProcess, const void* lpBaseAddress, size_t dwSize);
__declspec(dllimport) PVOID __stdcall GetCurrentProcess(void);

// InterlockedCompareExchangePointer through intrinsic
#define InterlockedCompareExchangePointer(Target, Exchange, Comperand) \
    _InterlockedCompareExchangePointer((void* volatile*)(Target), (void*)(Exchange), (void*)(Comperand))

void* _InterlockedCompareExchangePointer(void* volatile* Target, void* Exchange, void* Comperand);
//unsigned char _BitScanForward(unsigned long* Index, unsigned long Mask);

#pragma intrinsic(_InterlockedCompareExchangePointer)
//#pragma intrinsic(_BitScanForward)

#define OP_RET  0xC3
#define OP_NOP  0x90
#define OP_INT3 0xCC

typedef struct {
    uint8_t* start;
    uint8_t* cur;
} X64Emitter;

__inline static void E_init(X64Emitter* e) {
    // don't allocate the entire arena just allocate a reasonable chunk
    e->start = (uint8_t*)JIT_ALLOC_CODE(4096); //or some reasonable size
    e->cur = e->start;
}
//safer version
/*static void E_init(X64Emitter* e, uint8_t* buffer) {
    if (buffer) {
        e->start = buffer;
    }
    else {
        e->start = (uint8_t*)JIT_ALLOC_CODE(ALLOC_ARENA_SIZE);
    }
    e->cur = e->start;
}*/

__inline static size_t E_pos(X64Emitter* e) {
    return (size_t)(e->cur - e->start);
}

__inline static void E_emit(X64Emitter* e, uint8_t b) {
    *(e->cur)++ = b;
}

/*
prevented UB with volatile
MSVC with optimizations can think that e->cur is uint8_t* and uint32_t* is useless
*/
__inline static void E_dword(X64Emitter* e, uint32_t d) {
    *(volatile uint32_t*)e->cur = d;
    e->cur += 4;
}

__inline static void E_qword(X64Emitter* e, uint64_t q) {
    *(uint64_t*)e->cur = q;
    e->cur += 8;
}

__inline static void E_bytes(X64Emitter* e, const uint8_t* b, size_t n) {
    for (size_t i = 0; i < n; i++)
        *(e->cur)++ = b[i];
}

__inline static void E_setb(X64Emitter* e, size_t pos, uint8_t v) {
    e->start[pos] = v;
}

__inline static void E_setd(X64Emitter* e, size_t pos, uint32_t v) {
    *(uint32_t*)(e->start + pos) = v;
}

// REX prefix
__inline static void E_rex(X64Emitter* e, int w, int r, int x, int b) {
    E_emit(e, 0x40 | (!!w << 3) | (!!r << 2) | (!!x << 1) | !!b);
}

// ModR/M byte
__inline static void E_modrm(X64Emitter* e, uint8_t mod, uint8_t reg, uint8_t rm) {
    E_emit(e, ((mod & 3) << 6) | ((reg & 7) << 3) | (rm & 7));
}

// SIB byte
__inline static void E_sib(X64Emitter* e, uint8_t scale, uint8_t index, uint8_t base) {
    E_emit(e, ((scale & 3) << 6) | ((index & 7) << 3) | (base & 7));
}

__inline static void E_flush(X64Emitter* e) {
    if (e->cur == e->start) return;
    JIT_FLUSH_CODE();
    FlushInstructionCache(GetCurrentProcess(), e->start, (SIZE_T)(e->cur - e->start));
    _mm_lfence();
}

//32bits
static int E_run_ret_int(X64Emitter* e) {
    if (e->cur == e->start) return 0;
    if (e->cur[-1] != OP_RET) E_emit(e, OP_RET);
    E_flush(e);
    return ((int(*)())e->start)();
}
//64bits
static long long E_run_ret_int64(X64Emitter* e) {
    if (e->cur == e->start) return 0;
    if (e->cur[-1] != OP_RET) E_emit(e, OP_RET);
    E_flush(e);

    //change mask of da type to (long long(*)())
    return ((long long(*)())e->start)();
}

// for returning float (XMM0)
static float E_run_ret_float(X64Emitter* e) {
    if (e->cur == e->start) return 0.0f;
    if (e->cur[-1] != OP_RET) E_emit(e, OP_RET);
    E_flush(e);
    return ((float(*)())e->start)();
}

// for returning void
static void E_run_ret_void(X64Emitter* e) {
    if (e->cur == e->start) return;
    if (e->cur[-1] != OP_RET) E_emit(e, OP_RET);
    E_flush(e);
    ((void(*)())e->start)();
}

__inline static void E_begin(X64Emitter* e) {
    e->cur = e->start;
}

__inline static void E_run(X64Emitter* e) {
    if (e->cur == e->start) return;
    if (e->cur[-1] != OP_RET) E_emit(e, OP_RET);
    E_flush(e);
    ((void(*)())e->start)();
}
// compiles and makes it cool well usable
__inline static void E_finalize(X64Emitter* e) {
    if (e->cur == e->start) return;
    if (e->cur[-1] != OP_RET) E_emit(e, OP_RET);
    E_flush(e);
}
/*
// some sheesh
#define E_EXECUTE(e, return_type, ...) \
    (((return_type(*)())((e)->start))(__VA_ARGS__))
*/

// with pointer sheesh (for ABI)
typedef void (*JIT_Function)(void* context);
__inline static void E_call(X64Emitter* e, void* context) {
    E_finalize(e);
    ((JIT_Function)e->start)(context);
}//da

// added for sheesh
#define E_RUN_RET(e, type) \
    (type)((type(*)())e->start)()

/*
fun fact:
1 cpu tact can use up to the 8 instructions per cycle
*/
/*
we need to somehow avoid frontend sheesh and make cpu work all the time and
not spinlocking nor waiting for smth to do
also we need to avoid B2B misses and cache misses
*/
/*
we need to add

Register allocation through graph coloring instead of linear scan
(more complex, for more complex stuff)

Instruction scheduling (instruction handling for cpu pipeline to eat betah)

Constant propagation (well as it says)

Inline caching (like in LuaJIT for faster repeating calls)

Binary translation (translation from ARM to x64, etc etc)
*/