/*
made for x64asm JIT library
Arena + Block + Stack + TLS support
good for it's purpose i guess
*/
#pragma once

//#include <windows.h> //deleted due to it's heavy butt
//include <stdint.h>
//#include <string.h> //deleted due to it's heavy butt
//#include <stddef.h>
#include <intrin.h>
#include "NoCRT.h"

#define MEM_COMMIT            0x00001000
#define MEM_RESERVE           0x00002000
#define MEM_RELEASE           0x00008000
#define PAGE_READWRITE        0x04
#define PAGE_EXECUTE_READ     0x20
#define PAGE_EXECUTE_READWRITE 0x40

#ifdef __cplusplus
extern "C" {
#endif

    //windows.h
    typedef unsigned long DWORD;
    typedef void* PVOID;

    __declspec(dllimport) PVOID __stdcall VirtualAlloc(PVOID lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect);
    __declspec(dllimport) int   __stdcall VirtualFree(PVOID lpAddress, size_t dwSize, DWORD dwFreeType);
    __declspec(dllimport) int   __stdcall VirtualProtect(PVOID lpAddress, size_t dwSize, DWORD flNewProtect, DWORD* lpflOldProtect);
    __declspec(dllimport) int   __stdcall FlushInstructionCache(PVOID hProcess, const void* lpBaseAddress, size_t dwSize);
    __declspec(dllimport) PVOID __stdcall GetCurrentProcess(void);

#define InterlockedCompareExchangePointer(Target, Exchange, Comperand) \
    _InterlockedCompareExchangePointer((void* volatile*)(Target), (void*)(Exchange), (void*)(Comperand))
    //unsigned char _BitScanForward(unsigned long*, unsigned long);
    //#pragma intrinsic(_InterlockedCompareExchangePointer)
    //#pragma intrinsic(_BitScanForward)

#define ALLOC_ARENA_SIZE (256 * 1024)//256KB per arena
#define ALLOC_MAX_BLOCKS 256
#define ALLOC_BITMAP_SIZE ((ALLOC_MAX_BLOCKS + 31) / 32)
#define ALLOC_ALIGN 16//16-byte alignment (for SSE/AVX), update to 64 for more safety

    static inline NCsize_t ALLOC_ALIGN_UP(NCsize_t size) {
        return (size + (ALLOC_ALIGN - 1)) & ~((NCsize_t)(ALLOC_ALIGN - 1));
    }

    static inline NCsize_t ALLOC_ALIGN_DOWN(NCsize_t size) {
        return size & ~((NCsize_t)(ALLOC_ALIGN - 1));
    }

    //ARENA ALLOCATOR Bump Allocator
    //1 time videleniya, reset all of the arena at once
    //1-2 instructions on alloc, 1 on reset
    typedef struct {
        uint8_t* base;//beginning
        uint8_t* ptr;//current pos
        uint8_t* end;
        NCsize_t   total;
        NCsize_t   peak;
        int      lock;      //0=unlocked, 1=locked (for cooler threads processing)
    } Arena;
    //you're not fucking stupid i think
    static int Arena_init(Arena* a) {
        if (!a) return 0;
        NoCRT_memset(a, 0, sizeof(Arena));
        a->base = (uint8_t*)VirtualAlloc(NULL, ALLOC_ARENA_SIZE,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!a->base) return 0;
        a->ptr = a->base;
        a->end = a->base + ALLOC_ARENA_SIZE;
        a->total = ALLOC_ARENA_SIZE;
        return 1;
    }
    //initialization with fixed size
    static int Arena_init_sized(Arena* a, NCsize_t size) {
        if (!a) return 0;
        NoCRT_memset(a, 0, sizeof(Arena));
        size = ALLOC_ALIGN_UP(size);
        a->base = (uint8_t*)VirtualAlloc(NULL, size,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!a->base) return 0;
        a->ptr = a->base;
        a->end = a->base + size;
        a->total = size;
        return 1;
    }
    //safe but not fast
    static void* Arena_alloc(Arena* a, NCsize_t size) {
        if (!a || !a->base) return NULL;
        size = ALLOC_ALIGN_UP(size);

        uint8_t* old_ptr;
        uint8_t* new_ptr;
        do {
            old_ptr = a->ptr; // volatile read on x64 is atomic for aligned pointer
            new_ptr = old_ptr + size;
            if (new_ptr > a->end) return NULL;
        } while (_InterlockedCompareExchange64(
            (volatile long long*)&a->ptr,
            (long long)new_ptr,
            (long long)old_ptr) != (long long)old_ptr);

        NCsize_t used = (NCsize_t)(new_ptr - a->base);
        if (used > a->peak) a->peak = used;
        return old_ptr;
    }
    //not safe but fast
    static void* Arena_alloc_fast(Arena* a, NCsize_t size) {
        if (!a || !a->base) return NULL;
        size = ALLOC_ALIGN_UP(size);
        uint8_t* p = a->ptr;
        if (p + size > a->end) return NULL;
        a->ptr = p + size;

        NCsize_t used = (NCsize_t)(a->ptr - a->base);
        if (used > a->peak) a->peak = used;

        return p;
    }
    //reset all of memory
    static void Arena_reset(Arena* a) {
        if (!a || !a->base) return;
        a->ptr = a->base;
    }
    static void Arena_free(Arena* a) {
        if (!a || !a->base) return;
        VirtualFree(a->base, 0, MEM_RELEASE);
        NoCRT_memset(a, 0, sizeof(Arena));
    }
    //how many memory is used sir?
    static NCsize_t Arena_used(const Arena* a) {
        if (!a || !a->base) return 0;
        return (NCsize_t)(a->ptr - a->base);
    }
    //how many memory do you have?
    static NCsize_t Arena_available(const Arena* a) {
        if (!a || !a->base) return 0;
        return (NCsize_t)(a->end - a->ptr);
    }
    //change memory permissions(for jit JIT: RW -> RX)
    static int Arena_protect(Arena* a, DWORD protect) {
        if (!a || !a->base) return 0;
        DWORD old;
        return VirtualProtect(a->base, a->total, protect, &old) != 0;
    }

    //FIXED SIZE POOL
    //alloc/free objects with the same size
    //O(1) alloc bitmap sheesh possible
    typedef struct {
        NCuint8_t* base;
        NCuint32_t  used[ALLOC_BITMAP_SIZE];
        NCsize_t    block_size;
        int       num_blocks;
        int       free_count;
        int       alloc_count;
        int       peak_count;
    } BlockAlloc;

    //initialize pool block
    static int BlockAlloc_init(BlockAlloc* b, NCsize_t block_size, int num_blocks) {
        if (!b) return 0;
        NoCRT_memset(b, 0, sizeof(BlockAlloc));

        b->block_size = ALLOC_ALIGN_UP(block_size);
        b->num_blocks = (num_blocks < ALLOC_MAX_BLOCKS) ? num_blocks : ALLOC_MAX_BLOCKS;

        NCsize_t total = b->block_size * b->num_blocks;
        b->base = (NCuint8_t*)VirtualAlloc(NULL, total,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!b->base) return 0;

        b->free_count = b->num_blocks;
        return 1;
    }

    //fast finding bit sheesh
    static void* BlockAlloc_alloc(BlockAlloc* b) {
        if (!b || !b->base || b->free_count == 0) return NULL;

        //fast finding first free bit through cpu
        for (int word = 0; word < ALLOC_BITMAP_SIZE; word++) {
            NCuint32_t w = b->used[word];
            if (w != 0xFFFFFFFF) { //if word contains at least one free bit

                unsigned long bit = 0;
                //~w inverts bits: free (0) inprocess (1). find first 1
                if (_BitScanForward(&bit, ~w)) {
                    int index = word * 32 + (int)bit;
                    if (index >= b->num_blocks) return NULL;

                    NCuint32_t mask = 1U << bit;

                    //make block busy
                    b->used[word] |= mask;
                    b->free_count--;
                    b->alloc_count++;
                    if (b->num_blocks - b->free_count > b->peak_count) {
                        b->peak_count = b->num_blocks - b->free_count;
                    }

                    return b->base + (index * b->block_size);
                }
            }
        }
        return NULL;
    }
    //only if you know what's the number
    static void* BlockAlloc_alloc_at(BlockAlloc* b, int index) {
        if (!b || !b->base || index < 0 || index >= b->num_blocks) return NULL;

        int word = index / 32;
        int bit = index % 32;
        uint32_t mask = 1U << bit;

        if (b->used[word] & mask) return NULL;//fuck you i am busy

        b->used[word] |= mask;
        b->free_count--;
        b->alloc_count++;

        return b->base + (index * b->block_size);
    }

    static void BlockAlloc_free(BlockAlloc* b, void* ptr) {
        if (!b || !b->base || !ptr) return;

        ptrdiff_t offset = (uint8_t*)ptr - b->base;
        //FIXED: cutter of negative offset before getting what's left
        // saving from lying cast into big size_t
        if (offset < 0) return;
        if ((size_t)offset % b->block_size != 0) return;

        int index = (int)(offset / b->block_size);
        if (index >= b->num_blocks) return;

        int word = index / 32;
        int bit = index % 32;
        NCuint32_t mask = 1U << bit;

        if (b->used[word] & mask) {
            b->used[word] &= ~mask;
            b->free_count++;
        }
    }

    //get index through variable 
    static int BlockAlloc_index(const BlockAlloc* b, const void* ptr) {
        if (!b || !b->base || !ptr) return -1;
        ptrdiff_t offset = (const NCuint8_t*)ptr - b->base;
        if (offset < 0 || (NCsize_t)offset % b->block_size != 0) return -1;
        int index = (int)(offset / b->block_size);
        return (index < b->num_blocks) ? index : -1;
    }
    static void BlockAlloc_free_pool(BlockAlloc* b) {
        if (!b || !b->base) return;
        VirtualFree(b->base, 0, MEM_RELEASE);
        NoCRT_memset(b, 0, sizeof(BlockAlloc));
    }
    //without memory reset
    static void BlockAlloc_reset(BlockAlloc* b) {
        if (!b) return;
        NoCRT_memset(b->used, 0, sizeof(b->used));
        b->free_count = b->num_blocks;
        b->alloc_count = 0;
    }

    //temp bufers and stack sheesh i guess
    //O(1) alloc, O(1) free through pop
    typedef struct {
        NCuint8_t* base;
        NCuint8_t* ptr;
        NCuint8_t* end;
        NCuint8_t* marks[ALLOC_MAX_BLOCKS];
        int      mark_count;
        NCsize_t   peak;
    } StackAlloc;
    static int StackAlloc_init(StackAlloc* s) {
        if (!s) return 0;
        NoCRT_memset(s, 0, sizeof(StackAlloc));
        s->base = (NCuint8_t*)VirtualAlloc(NULL, ALLOC_ARENA_SIZE,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!s->base) return 0;
        s->ptr = s->base;
        s->end = s->base + ALLOC_ARENA_SIZE;
        return 1;
    }
    static void* StackAlloc_alloc(StackAlloc* s, NCsize_t size) {
        if (!s || !s->base) return NULL;
        size = ALLOC_ALIGN_UP(size);
        if (s->ptr + size > s->end) return NULL;

        void* p = s->ptr;
        s->ptr += size;

        NCsize_t used = (NCsize_t)(s->ptr - s->base);
        if (used > s->peak) s->peak = used;

        return p;
    }
    static int StackAlloc_mark(StackAlloc* s) {
        if (!s || s->mark_count >= ALLOC_MAX_BLOCKS) return 0;
        s->marks[s->mark_count++] = s->ptr;
        return 1;
    }
    static void StackAlloc_pop(StackAlloc* s) {
        if (!s || s->mark_count == 0) return;
        s->ptr = s->marks[--s->mark_count];
    }
    //delete all above(not TODO sheesh, i think that i am not that stupid and someone who reads this too)
    static void StackAlloc_pop_to(StackAlloc* s, int mark_index) {
        if (!s || mark_index < 0 || mark_index >= s->mark_count) return;
        s->ptr = s->marks[mark_index];
        // FIXED: counter of smth is equal to mark_index. 
        //well deletes and writes new
        s->mark_count = mark_index;
    }

    static void StackAlloc_reset(StackAlloc* s) {
        if (!s) return;
        s->ptr = s->base;
        s->mark_count = 0;
    }

    static void StackAlloc_free(StackAlloc* s) {
        if (!s || !s->base) return;
        VirtualFree(s->base, 0, MEM_RELEASE);
        NoCRT_memset(s, 0, sizeof(StackAlloc));
    }

    typedef struct {
        Arena  arena;
        int    is_executable;
    } JitArena;
    //function above was changed due to Data Execution Prevention and immediate crash
    /*//RW then make it RX
    static int JitArena_init(JitArena* j) {
        if (!j) return 0;
        NoCRT_memset(j, 0, sizeof(JitArena));

        //RW
        j->arena.base = (uint8_t*)VirtualAlloc(NULL, ALLOC_ARENA_SIZE,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!j->arena.base) return 0;

        j->arena.ptr = j->arena.base;
        j->arena.end = j->arena.base + ALLOC_ARENA_SIZE;
        j->arena.total = ALLOC_ARENA_SIZE;

        //RWX
        DWORD old;
        VirtualProtect(j->arena.base, ALLOC_ARENA_SIZE, PAGE_EXECUTE_READWRITE, &old);

        j->is_executable = 1;
        return 1;
    }*/
    static int JitArena_init(JitArena* j) {
        if (!j) return 0;
        NoCRT_memset(j, 0, sizeof(JitArena));

        // Use RWX for now to eliminate protection issues
        j->arena.base = (NCuint8_t*)VirtualAlloc(NULL, ALLOC_ARENA_SIZE,
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!j->arena.base) return 0;

        j->arena.ptr = j->arena.base;
        j->arena.end = j->arena.base + ALLOC_ARENA_SIZE;
        j->arena.total = ALLOC_ARENA_SIZE;
        j->is_executable = 1;
        return 1;
    }

    // finalizing sheesh (in E_run/E_finalize)
    static int JitArena_finalize(JitArena* j, NCsize_t code_size) {
        if (!j || j->is_executable) return 0;
        DWORD old;
        // change only used part (well not da full page)
        if (!VirtualProtect(j->arena.base, code_size, PAGE_EXECUTE_READ, &old)) {
            return 0;
        }
        FlushInstructionCache(GetCurrentProcess(), j->arena.base, code_size);
        j->is_executable = 1;
        return 1;
    }

    //for well writing it again
    static int JitArena_make_writable(JitArena* j) {
        DWORD old;
        return VirtualProtect(j->arena.base, j->arena.total, PAGE_READWRITE, &old) != 0;
    }

    //RW
    /*static void* JitArena_alloc_code(JitArena* j, size_t size) {
        return Arena_alloc_fast(&j->arena, size);
    }*/
    //added safety, well if you know when page is writable then uncomment thing
    //that right above that comment
    static void* JitArena_alloc_code(JitArena* j, NCsize_t size) {
        if (!j) return NULL;
        // If currently executable, make writable first
        if (j->is_executable) {
            JitArena_make_writable(j);
            j->is_executable = 0;  // Mark as not executable since we just made it writable
        }
        return Arena_alloc_fast(&j->arena, size);
    }

    //for RX
    static int JitArena_make_executable(JitArena* j) {
        if (!j || !j->arena.base) return 0;
        DWORD old;
        //RW then RX
        if (!VirtualProtect(j->arena.base, j->arena.total, PAGE_EXECUTE_READ, &old)) {
            return 0;
        }
        FlushInstructionCache(GetCurrentProcess(), j->arena.base, j->arena.total);
        return 1;
    }
    //this shit for RW
    static void JitArena_reset(JitArena* j) {
        if (!j) return;
        //just do this because PAGE_READWRITE is always like that
        Arena_reset(&j->arena);
    }
    static void JitArena_free(JitArena* j) {
        if (!j) return;
        Arena_free(&j->arena);
        j->is_executable = 0;
    }

    /*
    you can delete it or smth if you're using it, i'm too lazy to make it private
    or idk, i need to add that into README
    */
    typedef struct {
        JitArena    code_arena;
        Arena       data_arena;
        StackAlloc  temp_stack;
        BlockAlloc  f16_vec_pool;
        volatile int initialized; //volative blah blah blah new safety shit hooray
    } JitMemoryManager;

    static JitMemoryManager g_jit_mem;

    static int JitMemory_init(void) {
        if (g_jit_mem.initialized) return 1;

        //get da init
        if (_InterlockedCompareExchange((volatile long*)&g_jit_mem.initialized, -1, 0) != 0) {
            //other stream is initting it, then we watch and wait
            while (!g_jit_mem.initialized || g_jit_mem.initialized == -1) {
                _mm_pause();
            }
            return g_jit_mem.initialized > 0 ? 1 : 0;
        }

        //we get it, we init it
        if (!JitArena_init(&g_jit_mem.code_arena)) {
            g_jit_mem.initialized = 0;
            return 0;
        }
        if (!Arena_init_sized(&g_jit_mem.data_arena, 1024 * 1024)) {
            JitArena_free(&g_jit_mem.code_arena);
            g_jit_mem.initialized = 0;
            return 0;
        }
        if (!StackAlloc_init(&g_jit_mem.temp_stack)) {
            JitArena_free(&g_jit_mem.code_arena);
            Arena_free(&g_jit_mem.data_arena);
            g_jit_mem.initialized = 0;
            return 0;
        }
        if (!BlockAlloc_init(&g_jit_mem.f16_vec_pool, 256, 256)) {
            JitArena_free(&g_jit_mem.code_arena);
            Arena_free(&g_jit_mem.data_arena);
            StackAlloc_free(&g_jit_mem.temp_stack);
            g_jit_mem.initialized = 0;
            return 0;
        }

        _InterlockedExchange((volatile long*)&g_jit_mem.initialized, 1);
        return 1;
    }
    static void JitMemory_shutdown(void) {
        if (!g_jit_mem.initialized || g_jit_mem.initialized == -1) return;

        //get on shutdown
        if (_InterlockedCompareExchange((volatile long*)&g_jit_mem.initialized, -2, 1) != 1) {
            return; //other stream is already shotdown or initting it
        }

        JitArena_free(&g_jit_mem.code_arena);
        Arena_free(&g_jit_mem.data_arena);
        StackAlloc_free(&g_jit_mem.temp_stack);
        BlockAlloc_free_pool(&g_jit_mem.f16_vec_pool);
        NoCRT_memset(&g_jit_mem, 0, sizeof(g_jit_mem));
        g_jit_mem.initialized = 0;
    }
    static void JitMemory_reset_frame(void) {
        StackAlloc_reset(&g_jit_mem.temp_stack);//don't reset arena, i don't remember why, i remembered for jit using
    }
    //fuck yall, well i think you can already understand from the code what this shit does
    static void JitMemory_reset_all(void) {
        JitArena_reset(&g_jit_mem.code_arena);
        Arena_reset(&g_jit_mem.data_arena);
        StackAlloc_reset(&g_jit_mem.temp_stack);
        BlockAlloc_reset(&g_jit_mem.f16_vec_pool);
    }

    //macroses
#define JIT_MAKE_WRITABLE()       JitArena_make_writable(&g_jit_mem.code_arena)
#define JIT_ALLOC_CODE(size)      JitArena_alloc_code(&g_jit_mem.code_arena, (size))
#define JIT_ALLOC_DATA(size)      Arena_alloc_fast(&g_jit_mem.data_arena, (size))
#define JIT_ALLOC_TEMP(size)      StackAlloc_alloc(&g_jit_mem.temp_stack, (size))
#define JIT_ALLOC_F16_VEC()       ((uint16_t*)BlockAlloc_alloc(&g_jit_mem.f16_vec_pool))
#define JIT_FREE_F16_VEC(ptr)     BlockAlloc_free(&g_jit_mem.f16_vec_pool, (ptr))
#define JIT_TEMP_MARK()           StackAlloc_mark(&g_jit_mem.temp_stack)
#define JIT_TEMP_POP()            StackAlloc_pop(&g_jit_mem.temp_stack)
#define JIT_FLUSH_CODE()          JitArena_make_executable(&g_jit_mem.code_arena)


//statistics
    typedef struct {
        size_t code_used;
        size_t code_peak;
        size_t data_used;
        size_t data_peak;
        size_t temp_peak;
        int    f16_vecs_used;
        int    f16_vecs_peak;
    } JitMemoryStats;

    static void JitMemory_get_stats(JitMemoryStats* stats) {
        if (!stats) return;
        NoCRT_memset(stats, 0, sizeof(JitMemoryStats));
        stats->code_used = Arena_used(&g_jit_mem.code_arena.arena);
        stats->code_peak = g_jit_mem.code_arena.arena.peak;
        stats->data_used = Arena_used(&g_jit_mem.data_arena);
        stats->data_peak = g_jit_mem.data_arena.peak;
        stats->temp_peak = g_jit_mem.temp_stack.peak;
        stats->f16_vecs_used = g_jit_mem.f16_vec_pool.num_blocks - g_jit_mem.f16_vec_pool.free_count;
        stats->f16_vecs_peak = g_jit_mem.f16_vec_pool.peak_count;
    }

#ifdef __cplusplus
}
#endif
