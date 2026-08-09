/*
 * MSVC x64 only header file because of da some specific sheesh, but if you can
 * make it on other compilers feel free to use it, but mention me after all
 * didn't ya see da licensE?
*/
/*
USEFUL SOURCES:
AVX512 auto throttling: https://travisdowns.github.io/blog/2020/01/17/avxfreq1.html
i forgot to add other sheesh
*/
// FEATURE FLAGS
// #define NOCRT_NO_MEMORY     // remove memset/memcpy/memmove/memcmp/memchr
// #define NOCRT_NO_STRINGS    // remove strlen/strcpy/strcmp/strchr/strstr
// #define NOCRT_NO_CONVERSION // remove atoi/atof/itoa/ftoa
// #define NOCRT_NO_RANDOM     // remove rand
// #define NOCRT_NO_MATH       // remove rsqrt/rcp/floor/round
// #define NOCRT_NO_ATOMIC     // remove atomics
// #define NOCRT_NO_HASH       // remove CRC32 hash
// #define NOCRT_NO_PREFETCH   // remove prefetch
// #define NOCRT_NO_AVX512     // remove AVX-512
// #define NOCRT_NO_AVX2       // remove AVX2
// #define NOCRT_LEAN_AND_MEAN // don't use anything except scalar fallbacks
#pragma strict_gs_check
#pragma once
//#include <stdint.h>
//#include <stddef.h>
#include <intrin.h>   //MSVC intrinsics
#include <immintrin.h> //SSE/AVX
#include <emmintrin.h>

/*#ifdef __AVX512F__
if (NoCRT_cpu_avx512) {
    __m512i zero = _mm512_setzero_si512();
    while (1) {
        __m512i v = _mm512_loadu_si512((const __m512i*)s);
        __mmask64 mask = _mm512_cmpeq_epi8_mask(v, zero);
        if (mask) {
            return (s - start) + __lzcnt64(mask);  // trailing zero count
        }
        s += 64;
    }
}
#endif
*/

#define NOCRT_MOD_POW2(x, pow2) ((x) & ((pow2) - 1))
typedef unsigned __int64 NCuint64_t;   // 64bit (8byte)
typedef unsigned __int32 NCuint32_t;  // 32bit (4bytes)
typedef unsigned __int16 NCuint16_t; // 16bits  (2vytes)
typedef unsigned __int8  NCuint8_t; // 8bit   (1byte)
typedef                     NCint8_t; //ye
typedef short               NCint16_t; //ye
typedef int                 NCint32_t; //ye
typedef long long           NCint64_t; //ye
typedef unsigned __int64    NCsize_t;  //works only on x64
/*FOR FUTURE
typedef unsigned long long NCuint64_t;//64bit almost everywhere
typedef unsigned int    NCuint32_t;//32bits on everywhere
typedef unsigned short  NCuint16_t;//16bit on everywhere
typedef unsigned char   NCuint8_t; // 8bit everywhere
*/

typedef char* va_list;
__declspec(dllimport) int __stdcall WriteConsoleA(void* hConsole, const void* lpBuffer, unsigned long nChars, unsigned long* lpCharsWritten, void* lpReserved);
__declspec(dllimport) void* __stdcall GetStdHandle(unsigned long nStdHandle);
#define STD_OUTPUT_HANDLE ((unsigned long)-11)

// 2. Define the macros mapping to MSVC's internal compiler built-ins
#define va_start(ap, x) __crt_va_start(ap, x)
#define va_arg(ap, t)   __crt_va_arg(ap, t)
#define va_end(ap)     __crt_va_end(ap)

static inline int NoCRT_CPU_SSE2(void) {
    int cpuInfo[4];
    __cpuid(cpuInfo, 1);
    return (cpuInfo[3] & (1 << 26)) != 0;
}

static inline int NoCRT_CPU_AVX2(void) {
    int cpuInfo[4];
    __cpuid(cpuInfo, 1);
    if (!(cpuInfo[2] & (1 << 28))) return 0; // xgetbv support
    if ((_xgetbv(0) & 6) != 6) return 0; //OS saves YMM
    __cpuid(cpuInfo, 7);
    return (cpuInfo[1] & (1 << 5)) != 0;
}

#ifdef __AVX512F__
static inline int NoCRT_CPU_AVX512F(void) {
    int cpuInfo[4];
    __cpuid(cpuInfo, 1);
    if (!(cpuInfo[2] & (1 << 28))) return 0;
    if ((_xgetbv(0) & 0xE6) != 0xE6) return 0;
    __cpuid(cpuInfo, 7);
    return (cpuInfo[1] & (1 << 16)) != 0;
}
#else
static inline int NoCRT_CPU_AVX512F(void) { return 0; }
#endif

//cache on first execution
//static int NoCRT_cpu_features_cached = 0;
//static int NoCRT_cpu_sse2 = 0, NoCRT_cpu_avx2 = 0, NoCRT_cpu_avx512 = 0;

extern volatile long NoCRT_cpu_features_cached;
extern int NoCRT_cpu_sse2;
extern int NoCRT_cpu_avx2;
extern int NoCRT_cpu_avx512;

// heavy init func, starts only on start
// don't mess with it he have __declspec(noinline)), for SIMD
__declspec(noinline) static void NoCRT_init_cpu_features_slow(void) {
    //get this from 0 (Not Ready) to 1 (In Progress).
    //only 1 thread will use CPUID
    long expected = 0;
    long status = _InterlockedCompareExchange(&NoCRT_cpu_features_cached, 1, expected);

    if (status == 0) {
        // i've won this race i'm gonna init
        int sse2 = NoCRT_CPU_SSE2();
        int avx2 = NoCRT_CPU_AVX2();
        int avx512 = NoCRT_CPU_AVX512F();

        NoCRT_cpu_sse2 = sse2;
        NoCRT_cpu_avx2 = avx2;
        NoCRT_cpu_avx512 = avx512;

        // for flags writing
        _ReadWriteBarrier();

        //do status 2 (ready) 
        // Interlocked-operation will create needed Hardware Memory Barrier.
        _InterlockedExchange(&NoCRT_cpu_features_cached, 2);
    }
    else {
        //if status == 1, then other fella uses CPUID right now
        //wait(Spin-wait), until it's 2.
        while (_InterlockedOr((volatile long*)&NoCRT_cpu_features_cached, 0) != 2) {
            _mm_pause();//for cpu overload issue
        }
    }
    /*or change that else with this for more safety
    else {
    // wait without LOCK, just read memory, make first thread work patiently
    while (*(volatile long*)&NoCRT_cpu_features_cached != 2) {
        _mm_pause(); // safety for cpu to not overheat for some reason
    }
}
    */
}

static inline void NoCRT_init_cpu_features(void) {
    //Fast-path: if status == 2, then we can read flags safely
    //compiler barrier for reordering reading flags of cpu
    if (_InterlockedOr((volatile long*)&NoCRT_cpu_features_cached, 0) != 2) {
        NoCRT_init_cpu_features_slow();
    }
}
/*
more safe version:
works like instead of locking randomly(i don't know why) it just makes mov
instruction:
static inline void NoCRT_init_cpu_features(void) {
    //simple atomarn reading through memory without LOCK prefix
    long status = *(volatile long*)&NoCRT_cpu_features_cached;

    if (status != 2) {
        NoCRT_init_cpu_features_slow();
    }
}
*/

//memset with rep stosb (cool on x64)
/*static inline void NoCRT_memset(void* ptr, int value, size_t num) {
    __stosb((unsigned char*)ptr, (unsigned char)value, num);
}*/
typedef void (*NoCRT_memset_t)(void*, int, NCsize_t);
typedef void (*NoCRT_memcpy_t)(void*, const void*, NCsize_t);
typedef void (*NoCRT_memmove_t)(void*, const void*, NCsize_t);

static NoCRT_memset_t  NoCRT_memset_override = NULL;
static NoCRT_memcpy_t  NoCRT_memcpy_override = NULL;
static NoCRT_memmove_t NoCRT_memmove_override = NULL;

#define NOCRT_OVERRIDE_MEMSET(fn)  (NoCRT_memset_override  = (fn))
#define NOCRT_OVERRIDE_MEMCPY(fn)  (NoCRT_memcpy_override  = (fn))
#define NOCRT_OVERRIDE_MEMMOVE(fn) (NoCRT_memmove_override = (fn))

static inline void NoCRT_memset(void* ptr, int value, NCsize_t num) {
    if (NoCRT_memset_override) { NoCRT_memset_override(ptr, value, num); return; }
    __stosb((unsigned char*)ptr, (unsigned char)value, num);
}

static inline void NoCRT_memset_nt(void* ptr, int value, NCsize_t num) {
    // for lil buffers NT instructions are bad( (bcuz of cache sheesh)
    // REP STOSB (ERMS) on lil and medium blocks will work fine
    if (num < 4096) {
        __stosb((unsigned char*)ptr, (unsigned char)value, num);
        return;
    }

    NoCRT_init_cpu_features();
    unsigned char* p = (unsigned char*)ptr;
    unsigned char val8 = (unsigned char)value;

    // AVX-512
    if (NoCRT_cpu_avx512) {
        // align to 64 bytes to prevent hardware errors and #GP
        while (((uintptr_t)p & 63) != 0 && num > 0) {
            *p++ = val8;
            num--;
        }

        // do shit after aligning, i am a dumbass
        NCsize_t n512 = num >> 9;  // how many blocks with 512 byte(8amount*64byte)
        NCsize_t rem = num & 511; //what remains after da big blocks

        if (n512) {
            __m512i v = _mm512_set1_epi8((char)value);
            for (NCsize_t i = 0; i < n512; i++) {
                _mm512_stream_si512((__m512i*)(p + 0), v);
                _mm512_stream_si512((__m512i*)(p + 64), v);
                _mm512_stream_si512((__m512i*)(p + 128), v);
                _mm512_stream_si512((__m512i*)(p + 192), v);
                _mm512_stream_si512((__m512i*)(p + 256), v);
                _mm512_stream_si512((__m512i*)(p + 320), v);
                _mm512_stream_si512((__m512i*)(p + 384), v);
                _mm512_stream_si512((__m512i*)(p + 448), v);
                p += 512;
            }
        }

        //fill what remains with blocks to 64bytes
        NCsize_t n64 = rem >> 6;
        if (n64) {
            __m512i v = _mm512_set1_epi8((char)value);
            for (NCsize_t i = 0; i < n64; i++) {
                _mm512_stream_si512((__m512i*)p, v);
                p += 64;
            }
        }

        // give tail to stosb
        NCsize_t tail = rem & 63;
        if (tail) {
            __stosb(p, val8, tail);
        }
    }
    //AVX2
    else if (NoCRT_cpu_avx2) {
        //aling to 32bytes
        while (((uintptr_t)p & 31) != 0 && num > 0) {
            *p++ = val8;
            num--;
        }

        // count after align fuck
        NCsize_t n256 = num >> 8;  //blocks to 256 bytes(8amount * 32bytes)
        NCsize_t rem = num & 255;

        if (n256) {
            __m256i v = _mm256_set1_epi8((char)value);
            for (NCsize_t i = 0; i < n256; i++) {
                _mm256_stream_si256((__m256i*)(p + 0), v);
                _mm256_stream_si256((__m256i*)(p + 32), v);
                _mm256_stream_si256((__m256i*)(p + 64), v);
                _mm256_stream_si256((__m256i*)(p + 96), v);
                _mm256_stream_si256((__m256i*)(p + 128), v);
                _mm256_stream_si256((__m256i*)(p + 160), v);
                _mm256_stream_si256((__m256i*)(p + 192), v);
                _mm256_stream_si256((__m256i*)(p + 224), v);
                p += 256;
            }
        }

        // blocks to 32bytes
        NCsize_t n32 = rem >> 5;
        if (n32) {
            __m256i v = _mm256_set1_epi8((char)value);
            for (NCsize_t i = 0; i < n32; i++) {
                _mm256_stream_si256((__m256i*)p, v);
                p += 32;
            }
        }

        // well tail duh
        NCsize_t tail = rem & 31;
        if (tail) {
            __stosb(p, val8, tail);
        }
    }
    // SSE2
    else if (NoCRT_cpu_sse2) {
        // align to 16bytes
        while (((uintptr_t)p & 15) != 0 && num > 0) {
            *p++ = val8;
            num--;
        }

        // do this after aligning damn it
        NCsize_t n256 = num >> 8;  // blocks to 256bytes(16amount * 16byte)
        NCsize_t rem = num & 255;

        if (n256) {
            __m128i v = _mm_set1_epi8((char)value);
            for (NCsize_t i = 0; i < n256; i++) {
                _mm_stream_si128((__m128i*)(p + 0), v);   _mm_stream_si128((__m128i*)(p + 16), v);
                _mm_stream_si128((__m128i*)(p + 32), v);  _mm_stream_si128((__m128i*)(p + 48), v);
                _mm_stream_si128((__m128i*)(p + 64), v);  _mm_stream_si128((__m128i*)(p + 80), v);
                _mm_stream_si128((__m128i*)(p + 96), v);  _mm_stream_si128((__m128i*)(p + 112), v);
                _mm_stream_si128((__m128i*)(p + 128), v); _mm_stream_si128((__m128i*)(p + 144), v);
                _mm_stream_si128((__m128i*)(p + 160), v); _mm_stream_si128((__m128i*)(p + 176), v);
                _mm_stream_si128((__m128i*)(p + 192), v); _mm_stream_si128((__m128i*)(p + 208), v);
                _mm_stream_si128((__m128i*)(p + 224), v); _mm_stream_si128((__m128i*)(p + 240), v);
                p += 256;
            }
        }

        //blocks to 16byte
        NCsize_t n16 = rem >> 4;
        if (n16) {
            __m128i v = _mm_set1_epi8((char)value);
            for (NCsize_t i = 0; i < n16; i++) {
                _mm_stream_si128((__m128i*)p, v);
                p += 16;
            }
        }

        //well ye tail
        NCsize_t tail = rem & 15;
        if (tail) {
            __stosb(p, val8, tail);
        }
    }
    // old CPU fallback
    else {
        __stosb(p, val8, num);
    }

    // finalize well make sure that shit goes to RAM
    _mm_sfence();
}
/*
more optimized version

static inline void NoCRT_memset_nt(void* ptr, int value, NCsize_t num) {
    // ERMS (Enhanced REP STOSB) on Intel Ivy Bridge+ and AMD Zen3+
    // optimized for hardware sheesh and better for AVX blocks < 2-4kb.
    if (num < 2048) {
        __stosb((unsigned char*)ptr, (unsigned char)value, num);
        return;
    }

    NoCRT_init_cpu_features();
    unsigned char* p = (unsigned char*)ptr;
    unsigned char* end = p + num;
    unsigned char val8 = (unsigned char)value;

    // align pointer, instead of while cycle do other shit (faster for CPU)
    size_t align_mask = NoCRT_cpu_avx512 ? 63 : (NoCRT_cpu_avx2 ? 31 : 15);
    size_t misalignment = (uintptr_t)p & align_mask;
    if (misalignment) {
        size_t leading = (align_mask + 1) - misalignment;
        __stosb(p, val8, leading);
        p += leading;
    }

    // remaining for block sheesh
    size_t remaining = end - p;

    // main cycles (Non-Temporal)
    if (NoCRT_cpu_avx512) {
        __m512i v = _mm512_set1_epi8((char)value);
        // everything in one, per 256byte
        while (remaining >= 256) {
            _mm512_stream_si512((__m512i*)(p + 0), v);   _mm512_stream_si512((__m512i*)(p + 64), v);
            _mm512_stream_si512((__m512i*)(p + 128), v); _mm512_stream_si512((__m512i*)(p + 192), v);
            p += 256; remaining -= 256;
        }
        while (remaining >= 64) {
            _mm512_stream_si512((__m512i*)p, v);
            p += 64; remaining -= 64;
        }
    }
    else if (NoCRT_cpu_avx2) {
        __m256i v = _mm256_set1_epi8((char)value);
        while (remaining >= 256) {
            _mm256_stream_si256((__m256i*)(p + 0), v);   _mm256_stream_si256((__m256i*)(p + 32), v);
            _mm256_stream_si256((__m256i*)(p + 64), v);   _mm256_stream_si256((__m256i*)(p + 96), v);
            _mm256_stream_si256((__m256i*)(p + 128), v); _mm256_stream_si256((__m256i*)(p + 160), v);
            _mm256_stream_si256((__m256i*)(p + 192), v); _mm256_stream_si256((__m256i*)(p + 224), v);
            p += 256; remaining -= 256;
        }
        while (remaining >= 32) {
            _mm256_stream_si256((__m256i*)p, v);
            p += 32; remaining -= 32;
        }
    }
    else if (NoCRT_cpu_sse2) {
        __m128i v = _mm_set1_epi8((char)value);
        while (remaining >= 128) {
            _mm_stream_si128((__m128i*)(p + 0), v);  _mm_stream_si128((__m128i*)(p + 16), v);
            _mm_stream_si128((__m128i*)(p + 32), v); _mm_stream_si128((__m128i*)(p + 48), v);
            _mm_stream_si128((__m128i*)(p + 64), v); _mm_stream_si128((__m128i*)(p + 80), v);
            _mm_stream_si128((__m128i*)(p + 96), v); _mm_stream_si128((__m128i*)(p + 112), v);
            p += 128; remaining -= 128;
        }
        while (remaining >= 16) {
            _mm_stream_si128((__m128i*)p, v);
            p += 16; remaining -= 16;
        }
    }

    // tail duh (faster through __stosb)
    if (remaining) {
        __stosb(p, val8, remaining);
    }

    _mm_sfence();
}
*/

//memset 16/32/64 byte with SIMD
static inline void NoCRT_memset16(void* ptr, __m128i val) {
    _mm_storeu_si128((__m128i*)ptr, val);
}

static inline void NoCRT_memset32(void* ptr, __m256i val) {
    _mm256_storeu_si256((__m256i*)ptr, val);
}

static inline void NoCRT_memset64(void* ptr, __m512i val) {
    _mm512_storeu_si512((__m512i*)ptr, val);
}

//memcpy chooses the coolest path/way
static inline void NoCRT_memcpy(void* dst, const void* src, NCsize_t num) {
    if (num == 0) return;
    NoCRT_init_cpu_features();

    // for lil blocks (16byte) SSE
    // modern cpu's will likely turn this switch into some optimized shit
    if (num < 16) {
        unsigned char* d = (unsigned char*)dst;
        const unsigned char* s = (const unsigned char*)src;

        switch (num) {
        case 1:  *d = *s; return;
        case 2:  *(NCuint16_t*)d = *(const NCuint16_t*)s; return; // /Ob2 compiler optimizes safe-cast
        case 4:  *(NCuint32_t*)d = *(const NCuint32_t*)s; return;
        case 8:  *(NCuint64_t*)d = *(const NCuint64_t*)s; return;
        default: break;
        }
        // for non aligned of what remains (3, 5, 6, 7, 9-15) __movsb will be faster
        // so the sheesh above can be cooler
        __movsb(d, s, num);
        return;
    }

    // vector sheesh for big bad blocks
    // NOTICE: for AVX-512 blocks needed are from 128 to 256byte
    // or cpu can throttle a little bit (AVX throttling)
    if (NoCRT_cpu_avx512 && num >= 128) {
        size_t n64 = num >> 6;
        __m512i* d = (__m512i*)dst;
        const __m512i* s = (const __m512i*)src;
        for (size_t i = 0; i < n64; i++) {
            _mm512_storeu_si512(d + i, _mm512_loadu_si512(s + i));
        }
        num &= 63; // remainig tail
        if (num == 0) return;
        dst = (void*)(d + n64);
        src = (const void*)(s + n64);
        //_mm512_stream_si512((__m512i*)(d + i), _mm512_loadu_si512(s + i));
    }

    if (NoCRT_cpu_avx2 && num >= 32) {
        NCsize_t n32 = num >> 5;
        __m256i* d = (__m256i*)dst;
        const __m256i* s = (const __m256i*)src;
        for (NCsize_t i = 0; i < n32; i++) {
            _mm256_storeu_si256(d + i, _mm256_loadu_si256(s + i));
        }
        num &= 31;
        if (num == 0) return;
        dst = (void*)(d + n32);
        src = (const void*)(s + n32);
        //_mm256_stream_si256((__m256i*)(d + i), _mm256_loadu_si256(s + i));
    }

    if (NoCRT_cpu_sse2 && num >= 16) {
        NCsize_t n16 = num >> 4;
        __m128i* d = (__m128i*)dst;
        const __m128i* s = (const __m128i*)src;
        for (NCsize_t i = 0; i < n16; i++) {
            _mm_storeu_si128(d + i, _mm_loadu_si128(s + i));
        }
        num &= 15;
        if (num == 0) return;
        dst = (void*)(d + n16);
        src = (const void*)(s + n16);
        //_mm_stream_si128((__m128i*)(d + i), _mm_loadu_si128(s + i));
    }

    // tail (remainings), or just old cpus
    // if size > 64byte (on old CPUs), ERMS will turn on and it makes it faster
    // if this lil tail after AVX/SSE, __movsb will work great
    __movsb((unsigned char*)dst, (const unsigned char*)src, num);
}

static inline int NoCRT_memcmp(const void* a, const void* b, NCsize_t num) {
    if (!num) return 0;

    const unsigned char* p1 = (const unsigned char*)a;
    const unsigned char* p2 = (const unsigned char*)b;

    NoCRT_init_cpu_features();

    // AVX2 (blocks to 32byte)
    if (NoCRT_cpu_avx2 && num >= 32) {
        NCsize_t n32 = num >> 5;
        for (NCsize_t i = 0; i < n32; i++) {
            __m256i va = _mm256_loadu_si256((const __m256i*)p1);
            __m256i vb = _mm256_loadu_si256((const __m256i*)p2);

            // FIXED: mask now only uint32_t
            NCuint32_t mask = (NCuint32_t)_mm256_movemask_epi8(_mm256_cmpeq_epi8(va, vb));

            if (mask != 0xFFFFFFFF) {
                unsigned long offset;
                _BitScanForward(&offset, ~mask);
                return (int)p1[offset] - (int)p2[offset];
            }
            p1 += 32;
            p2 += 32;
        }
        num &= 31; // lowest tail from AVX2 (from 0 until 31bytes)
    }

    //SSE2 (here will be sheesh out of AVX2 or a lil bigger num)
    if (NoCRT_cpu_sse2 && num >= 16) {
        NCsize_t n16 = num >> 4;
        for (NCsize_t i = 0; i < n16; i++) {
            __m128i va = _mm_loadu_si128((const __m128i*)p1);
            __m128i vb = _mm_loadu_si128((const __m128i*)p2);

            // fixed mask for SSE2
            NCuint32_t mask = (NCuint32_t)_mm_movemask_epi8(_mm_cmpeq_epi8(va, vb)) & 0xFFFF;

            if (mask != 0xFFFF) {
                unsigned long offset;
                _BitScanForward(&offset, ~mask);
                return (int)p1[offset] - (int)p2[offset];
            }
            p1 += 16;
            p2 += 16;
        }
        num &= 15; // other smaller tail (from 0 until 15byte)
    }

    // byte sheesh somewhere 15 iterations
    while (num--) {
        if (*p1 != *p2) return (int)*p1 - (int)*p2;
        p1++; p2++;
    }
    return 0;
}

//memchr, finding byte through SIMD
static inline void* NoCRT_memchr(const void* ptr, int value, NCsize_t num) {
    if (!num) return NULL;

    const unsigned char* p = (const unsigned char*)ptr;
    unsigned char val8 = (unsigned char)value;
    NCsize_t remaining = num;

    NoCRT_init_cpu_features();

    //AVX2: aligning and finding
    if (NoCRT_cpu_avx2 && remaining >= 32) {
        while (((uintptr_t)p & 31) != 0 && remaining > 0) {
            if (*p == val8) return (void*)p;
            p++;
            remaining--;
        }

        if (remaining >= 32) {
            __m256i pattern = _mm256_set1_epi8((char)val8);
            NCsize_t n32 = remaining >> 5;
            remaining &= 31;

            for (NCsize_t i = 0; i < n32; i++) {
                __m256i v = _mm256_load_si256((const __m256i*)p);
                int mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(v, pattern));
                if (mask) {
#ifdef _MSC_VER
                    unsigned long offset;
                    _BitScanForward(&offset, mask);
#else
                    unsigned long offset = __builtin_ctz(mask);
#endif
                    return (void*)(p + offset);
                }
                p += 32;
            }
        }
    }

    //SSE2 aligning and founding
    if (NoCRT_cpu_sse2 && remaining >= 16) {
        // align to 16bytes (if AVX2 doesn't exists or tail is still exists somehow)
        while (((uintptr_t)p & 15) != 0 && remaining > 0) {
            if (*p == val8) return (void*)p;
            p++;
            remaining--;
        }

        if (remaining >= 16) {
            __m128i pattern = _mm_set1_epi8((char)val8);
            NCsize_t n16 = remaining >> 4;
            remaining &= 15;

            for (NCsize_t i = 0; i < n16; i++) {
                __m128i v = _mm_load_si128((const __m128i*)p); //align
                int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(v, pattern));
                if (mask) {
#ifdef _MSC_VER
                    unsigned long offset;
                    _BitScanForward(&offset, mask);
#else
                    unsigned long offset = __builtin_ctz(mask);
#endif
                    return (void*)(p + offset);
                }
                p += 16;
            }
        }
    }

    //tail
    while (remaining--) {
        if (*p == val8) return (void*)p;
        p++;
    }

    return NULL;
}

//strncpy with SSE optimization
/*static inline char* NoCRT_strncpy(char* dst, const char* src, size_t n) {
    if (!n) return dst;

    char* d = dst;
    const char* s = src;

    NoCRT_init_cpu_features();

    if (NoCRT_cpu_sse2 && n >= 16) {
        __m128i zero = _mm_setzero_si128();

        // Main SIMD loop: copy 16 bytes at a time
        while (n >= 16) {
            __m128i v = _mm_loadu_si128((const __m128i*)s);

            // Check if null terminator is in this block
            __m128i null_mask = _mm_cmpeq_epi8(v, zero);
            int null_pos = _mm_movemask_epi8(null_mask);

            if (null_pos) {
                // Found null terminator
                unsigned long idx;
                _BitScanForward(&idx, (unsigned long)null_pos);

                if ((size_t)idx < n) {
                    // Copy up to and including null, then zero-fill rest
                    size_t copy_len = idx + 1;

                    // Use movsb for exact copy to avoid overread
                    __movsb((unsigned char*)d, (const unsigned char*)s, copy_len);
                    d += copy_len;
                    n -= copy_len;

                    // Zero-fill remaining
                    if (n > 0) {
                        __stosb((unsigned char*)d, 0, n);
                    }
                    return dst;
                }
                else {
                    // Null is beyond n, copy all 16 bytes
                    _mm_storeu_si128((__m128i*)d, v);
                    d += 16;
                    s += 16;
                    n -= 16;
                }
            }
            else {
                // No null in this block, safe to copy all 16
                _mm_storeu_si128((__m128i*)d, v);
                d += 16;
                s += 16;
                n -= 16;
            }
        }
    }

    // Handle remaining bytes (n < 16 or no SSE2)
    while (n) {
        if (*s) {
            *d++ = *s++;
        }
        else {
            // source ended, zero-fill rest
            *d++ = '\0';
        }
        n--;
    }

    return dst;
}*/

// Maximally optimized strncpy with SSE 4-way unrolling
//strncpy with SSE optimization - corrected version
static inline char* NoCRT_strncpy(char* dst, const char* src, NCsize_t n) {
    if (!n) return dst;

    char* d = dst;
    const char* s = src;
    NCsize_t remaining = n;

    NoCRT_init_cpu_features();

    if (NoCRT_cpu_sse2 && remaining >= 16) {
        __m128i zero = _mm_setzero_si128();

        while (remaining >= 16) {
            __m128i v = _mm_loadu_si128((const __m128i*)s);

            // check for null terminator
            __m128i null_mask = _mm_cmpeq_epi8(v, zero);
            int mask = _mm_movemask_epi8(null_mask);

            if (mask) {
                // Null found in this 16-byte block
                unsigned long idx;
                _BitScanForward(&idx, (unsigned long)mask);

                NCsize_t copy_len = idx + 1; // Include null terminator

                if (copy_len <= remaining) {
                    // Copy up to and including null
                    __movsb((unsigned char*)d, (const unsigned char*)s, copy_len);
                    d += copy_len;
                    remaining -= copy_len;

                    // Zero-fill the rest
                    if (remaining > 0) {
                        __stosb((unsigned char*)d, 0, remaining);
                    }
                    return dst;
                }
                else {
                    // Null is beyond n, just copy remaining bytes
                    __movsb((unsigned char*)d, (const unsigned char*)s, remaining);
                    return dst;
                }
            }
            else {
                // No null in these 16 bytes
                _mm_storeu_si128((__m128i*)d, v);
                d += 16;
                s += 16;
                remaining -= 16;
            }
        }
    }

    // Handle remaining bytes (0 < remaining < 16 or no SSE2)
    if (remaining > 0) {
        NCsize_t i;
        for (i = 0; i < remaining && s[i] != '\0'; i++) {
            d[i] = s[i];
        }

        if (i < remaining) {
            // Null terminator found
            d[i] = '\0';
            i++;
            // Zero-fill remaining
            while (i < remaining) {
                d[i] = '\0';
                i++;
            }
        }
    }

    return dst;
}

//better performance
/*
//strncpy with SSE optimization - 64-byte unrolled version
static inline char* NoCRT_strncpy(char* dst, const char* src, size_t n) {
    if (!n) return dst;

    char* d = dst;
    const char* s = src;
    size_t remaining = n;

    NoCRT_init_cpu_features();

    if (NoCRT_cpu_sse2 && remaining >= 64) {
        __m128i zero = _mm_setzero_si128();

        // Process 64 bytes per iteration
        while (remaining >= 64) {
            __m128i v0 = _mm_loadu_si128((const __m128i*)(s));
            __m128i v1 = _mm_loadu_si128((const __m128i*)(s + 16));
            __m128i v2 = _mm_loadu_si128((const __m128i*)(s + 32));
            __m128i v3 = _mm_loadu_si128((const __m128i*)(s + 48));

            // Check all 4 blocks for null
            __m128i c0 = _mm_cmpeq_epi8(v0, zero);
            __m128i c1 = _mm_cmpeq_epi8(v1, zero);
            __m128i c2 = _mm_cmpeq_epi8(v2, zero);
            __m128i c3 = _mm_cmpeq_epi8(v3, zero);

            __m128i or01 = _mm_or_si128(c0, c1);
            __m128i or23 = _mm_or_si128(c2, c3);
            __m128i orall = _mm_or_si128(or01, or23);

            if (_mm_movemask_epi8(orall)) {
                // Found null - determine exact position
                size_t null_offset = 0;

                if (_mm_movemask_epi8(c0)) {
                    unsigned long idx;
                    _BitScanForward(&idx, (unsigned long)_mm_movemask_epi8(c0));
                    null_offset = idx;
                } else if (_mm_movemask_epi8(c1)) {
                    unsigned long idx;
                    _BitScanForward(&idx, (unsigned long)_mm_movemask_epi8(c1));
                    null_offset = 16 + idx;
                } else if (_mm_movemask_epi8(c2)) {
                    unsigned long idx;
                    _BitScanForward(&idx, (unsigned long)_mm_movemask_epi8(c2));
                    null_offset = 32 + idx;
                } else {
                    unsigned long idx;
                    _BitScanForward(&idx, (unsigned long)_mm_movemask_epi8(c3));
                    null_offset = 48 + idx;
                }

                size_t copy_len = null_offset + 1; // Include null

                if (copy_len <= remaining) {
                    __movsb((unsigned char*)d, (const unsigned char*)s, copy_len);
                    d += copy_len;
                    remaining -= copy_len;
                    if (remaining > 0) {
                        __stosb((unsigned char*)d, 0, remaining);
                    }
                    return dst;
                } else {
                    __movsb((unsigned char*)d, (const unsigned char*)s, remaining);
                    return dst;
                }
            }

            // No null found, copy all 64 bytes
            _mm_storeu_si128((__m128i*)(d), v0);
            _mm_storeu_si128((__m128i*)(d + 16), v1);
            _mm_storeu_si128((__m128i*)(d + 32), v2);
            _mm_storeu_si128((__m128i*)(d + 48), v3);

            d += 64;
            s += 64;
            remaining -= 64;
        }
    }

    // Handle remaining with SSE if >= 16 bytes
    if (NoCRT_cpu_sse2 && remaining >= 16) {
        __m128i zero = _mm_setzero_si128();

        while (remaining >= 16) {
            __m128i v = _mm_loadu_si128((const __m128i*)s);
            __m128i null_mask = _mm_cmpeq_epi8(v, zero);
            int mask = _mm_movemask_epi8(null_mask);

            if (mask) {
                unsigned long idx;
                _BitScanForward(&idx, (unsigned long)mask);
                size_t copy_len = idx + 1;

                if (copy_len <= remaining) {
                    __movsb((unsigned char*)d, (const unsigned char*)s, copy_len);
                    d += copy_len;
                    remaining -= copy_len;
                    if (remaining > 0) {
                        __stosb((unsigned char*)d, 0, remaining);
                    }
                    return dst;
                } else {
                    __movsb((unsigned char*)d, (const unsigned char*)s, remaining);
                    return dst;
                }
            }

            _mm_storeu_si128((__m128i*)d, v);
            d += 16;
            s += 16;
            remaining -= 16;
        }
    }

    // Handle final bytes (< 16 or no SSE2)
    if (remaining > 0) {
        size_t i = 0;
        while (i < remaining && s[i] != '\0') {
            d[i] = s[i];
            i++;
        }

        if (i < remaining) {
            d[i] = '\0';
            i++;
            while (i < remaining) {
                d[i] = '\0';
                i++;
            }
        }
    }

    return dst;
}
*/

static inline void NoCRT_memmove(void* dst, const void* src, NCsize_t num) {
    if (!num || dst == src) return;

    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;

    if (d < s || d >= s + num) {
        //no sheesh or dst next to src, then copy forward
        NoCRT_memcpy(dst, src, num);
    }
    else {
        //dst inside src, copy back
        d += num;
        s += num;
        while (num--) *--d = *--s;
    }
}

static inline void* NoCRT_memrchr(const void* ptr, int value, NCsize_t num) {
    if (!num) return NULL;

    const unsigned char* p = (const unsigned char*)ptr;
    const unsigned char* end = p + num;

    NoCRT_init_cpu_features();

    if (NoCRT_cpu_sse2 && num >= 16) {
        __m128i pattern = _mm_set1_epi8((char)value);

        //find by using idk 16 bytes
        while (num >= 16) {
            end -= 16;
            num -= 16;

            __m128i v = _mm_loadu_si128((const __m128i*)end);
            int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(v, pattern));

            if (mask) {
                // found it? now go and find da rightest bit
                unsigned long idx;
                _BitScanReverse(&idx, (unsigned long)mask);
                return (void*)(end + idx);
            }
        }
    }

    //or just basek way
    if (num > 0) {
        while (num--) {
            if (end[-1] == (unsigned char)value) return (void*)(end - 1);
            end--;
        }
    }

    return NULL;
}

static inline int NoCRT_itoa(int value, char* buf, int base) {
    if (!buf || base < 2 || base > 36) {
        if (buf) *buf = '\0';
        return 0;
    }

    char tmp[36];
    char* p = tmp;
    unsigned int v;
    int negative = 0;

    if (value < 0 && base == 10) {
        negative = 1;
        v = 0u - (unsigned int)value;  //|value| for non sign sheesh
    }
    else {
        v = (unsigned int)value;
    }

    do {
        unsigned int d = v % (unsigned int)base;
        *p++ = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        v /= (unsigned int)base;
    } while (v);

    if (negative) {
        *p++ = '-';
    }

    int len = (int)(p - tmp);
    while (p > tmp) {
        *buf++ = *--p;
    }
    *buf = '\0';
    return len;
}

//fized point sheesh
// helper micro func for unsigned long long (uint64_t)
static inline int NoCRT_itoa64(NCuint64_t value, char* buf) {
    char tmp[24];
    char* p = tmp;
    do {
        *p++ = (char)('0' + (value % 10));
        value /= 10;
    } while (value);

    int len = (int)(p - tmp);
    while (p > tmp) *buf++ = *--p;
    //*buf = '\0'; //not needed i think
    return len;
}

static inline int NoCRT_ftoa(float value, char* buf, int decimals) {
    char* start = buf;

    // -0.0 through bits shit for smth cool
    uint32_t float_bits = *(uint32_t*)&value;
    if (float_bits & 0x80000000) {
        *buf++ = '-';
        float_bits &= 0x7FFFFFFF;
        value = *(float*)&float_bits;
    }

    if (decimals > 9) decimals = 9;

    NCuint64_t pow10 = 1;
    for (int i = 0; i < decimals; i++) {
        pow10 *= 10;
    }

    // round it 1 time
    double val_d = (double)value + (0.5 / (double)pow10);

    NCuint64_t int_part = (NCuint64_t)val_d;
    double frac_double = val_d - (double)int_part;

    // just multiply because main shit is in da val_d
    NCuint64_t frac_part = (NCuint64_t)(frac_double * (double)pow10);

    if (frac_part >= pow10) {
        int_part++;
        frac_part = 0;
    }

    //write full shit
    buf += NoCRT_itoa64(int_part, buf);

    //write other shit
    if (decimals > 0) {
        *buf++ = '.';

        int digits_needed = 0;
        NCuint64_t temp = frac_part;
        do {
            digits_needed++;
            temp /= 10;
        } while (temp);

        if (frac_part == 0) digits_needed = 0;

        int missing_zeros = decimals - digits_needed;
        for (int i = 0; i < missing_zeros; i++) {
            *buf++ = '0';
        }

        if (frac_part > 0) {
            buf += NoCRT_itoa64(frac_part, buf);
        }
    }

    *buf = '\0';
    return (int)(buf - start);
}

/*
use pcmpistri instead of cmpeq + movemask for more speed
*/
//STRINGS and INTRINSICS
//strlen with repne scasb + SIMD
static inline char* NoCRT_strcpy(char* __restrict dst, const char* __restrict src) {
    char* d = dst;

    NoCRT_init_cpu_features();

    if (NoCRT_cpu_sse2) {
        __m128i zero = _mm_setzero_si128();

        while (1) {
            __m128i v = _mm_loadu_si128((const __m128i*)src);
            __m128i null_mask = _mm_cmpeq_epi8(v, zero);
            int mask = _mm_movemask_epi8(null_mask);

            if (mask) {
                // found null, copy up to and including it
                unsigned long idx;
                _BitScanForward(&idx, (unsigned long)mask);
                NCsize_t copy_len = idx + 1;
                __movsb((unsigned char*)d, (const unsigned char*)src, copy_len);
                break;
            }

            _mm_storeu_si128((__m128i*)d, v);
            d += 16;
            src += 16;
        }
        return dst;
    }

    // scalar fallback
    while (*src) *d++ = *src++;
    *d = '\0';
    return dst;
}

/*
POSSIBLE BUG:

if null terminator is on the first block right after the alignment, then you
readed 48 fat bytes ahead
if the line is right on the edge of the page(where is unmapped memory) will be
Access Violation

we use right now _mm_load_si128 on 4*16byte

SAME WITH NoCRT_strncpy, NoCRT_strchr_avx2
USE: Reading past the end of a string with masking
(https://www.agner.org/optimize/asmlib-instructions.pdf
3, 3.7
)
*/
static inline NCsize_t NoCRT_strlen(const char* s) {
    NoCRT_init_cpu_features();

    if (NoCRT_cpu_sse2) {
        const char* start = s;
        __m128i zero = _mm_setzero_si128();

        //safe byte to byte or byte per byte sheesh again for Page Fault bug
        while (((uintptr_t)s & 15) != 0) {
            if (*s == '\0') {
                return (NCsize_t)(s - start);
            }
            s++;
        }
        // now s is aligned to 16bye sheeesh

        // cycle to 64 byte per iteration
        while (1) {
            __m128i v0 = _mm_load_si128((const __m128i*)(s));
            __m128i v1 = _mm_load_si128((const __m128i*)(s + 16));
            __m128i v2 = _mm_load_si128((const __m128i*)(s + 32));
            __m128i v3 = _mm_load_si128((const __m128i*)(s + 48));

            __m128i c0 = _mm_cmpeq_epi8(v0, zero);
            __m128i c1 = _mm_cmpeq_epi8(v1, zero);
            __m128i c2 = _mm_cmpeq_epi8(v2, zero);
            __m128i c3 = _mm_cmpeq_epi8(v3, zero);

            __m128i or01 = _mm_or_si128(c0, c1);
            __m128i or23 = _mm_or_si128(c2, c3);
            __m128i orall = _mm_or_si128(or01, or23);

            if (_mm_movemask_epi8(orall)) {
                int m0 = _mm_movemask_epi8(c0);
                if (m0) {
                    unsigned long idx;
                    _BitScanForward(&idx, (unsigned long)m0);
                    return (NCsize_t)(s - start) + idx;
                }
                int m1 = _mm_movemask_epi8(c1);
                if (m1) {
                    unsigned long idx;
                    _BitScanForward(&idx, (unsigned long)m1);
                    return (NCsize_t)(s - start) + 16 + idx;
                }
                int m2 = _mm_movemask_epi8(c2);
                if (m2) {
                    unsigned long idx;
                    _BitScanForward(&idx, (unsigned long)m2);
                    return (NCsize_t)(s - start) + 32 + idx;
                }
                int m3 = _mm_movemask_epi8(c3);
                unsigned long idx;
                _BitScanForward(&idx, (unsigned long)m3);
                return (NCsize_t)(s - start) + 48 + idx;
            }
            s += 64;
        }
    }

    //fallback
    const char* p = s;
    while (*p) p++;
    return (NCsize_t)(p - s);
}

/*
add Agner Fog method sheesh "pcmpistri" instead of "cmpeq, movemask, bitscan"
(https://www.agner.org/optimize/instruction_tables.pdf
page 242
String instructions
PCMPESTRI x,x,i 8 8 4 4 SSE4.2
PCMPESTRI x,m128,i 8 7 1 4 SSE4.2
PCMPESTRM x,x,i 8 8 11-12 4 SSE4.2
PCMPESTRM x,m128,i 8 7 1 4 SSE4.2
PCMPISTRI x,x,i 3 3 3 3 SSE4.2
PCMPISTRI x,m128,i 4 3 1 3 SSE4.2
PCMPISTRM x,x,i 3 3 11 3 SSE4.2
PCMPISTRM x,m128,i 4 3 1 3 SSE4.2
)
*/
//strcmp with SIMD
static inline int NoCRT_strcmp(const char* a, const char* b) {
    NoCRT_init_cpu_features();

    if (NoCRT_cpu_sse2) {
        // safe byte by byte align for Page Fault bug
        //align pointer 'a' to 16byte sheesh
        while (((uintptr_t)a & 15) != 0) {
            if (*a != *b || *a == '\0') {
                return (unsigned char)*a - (unsigned char)*b;
            }
            a++;
            b++;
        }

        // main vector cycle
        __m128i zero = _mm_setzero_si128();

        while (1) {
            /*
            may be a potential page-crossing fault on b if it's near the end of
            a page
            */
            // va is safe (aligned), а vb can be not aligned,
            // but reading vb is safe for health, until we're not on the end of
            //the line 'a'
            __m128i va = _mm_load_si128((const __m128i*)a); // use load instead of loadu
            __m128i vb = _mm_loadu_si128((const __m128i*)b);

            //check matches of da bytes
            __m128i cmp_eq = _mm_cmpeq_epi8(va, vb);
            int mask_eq = _mm_movemask_epi8(cmp_eq);

            // if at least one nonmatchup (or 0 in one of the lines)
            if (mask_eq != 0xFFFF) {
                // invert mask of matches to find pos of the first nonmatchup
                unsigned int mask_diff = ~mask_eq & 0xFFFF;

                unsigned long idx;
                _BitScanForward(&idx, mask_diff);
                return (unsigned char)a[idx] - (unsigned char)b[idx];
            }

            // additional check if 'a' not ended (100% match, check on NULL)
            __m128i null_a = _mm_cmpeq_epi8(va, zero);
            int mask_null = _mm_movemask_epi8(null_a);
            if (mask_null != 0) {
                // absolutely matched and two of them are ended on \0
                return 0;
            }

            a += 16;
            b += 16;
        }
    }

    // fallback for non SSE2 cpus
    while (*a && *a == *b) { a++; b++; }
    return *(unsigned char*)a - *(unsigned char*)b;
}


// strchr with SIMD
/*static inline char* NoCRT_strchr(const char* s, int c) {
    NoCRT_init_cpu_features();

    if (NoCRT_cpu_sse2) {
        __m128i pattern = _mm_set1_epi8((char)c);
        __m128i zero = _mm_setzero_si128();

        while (1) {
            __m128i v = _mm_loadu_si128((const __m128i*)s);
            __m128i match = _mm_cmpeq_epi8(v, pattern);
            __m128i nulls = _mm_cmpeq_epi8(v, zero);

            //combine: found symbol or get to the end of the line
            __m128i result = _mm_or_si128(match, nulls);
            int mask = _mm_movemask_epi8(result);

            if (mask) {
                unsigned long idx;
                _BitScanForward(&idx, (unsigned long)mask);

                //if null terminator meets sooner or true(or smth)
                if (s[idx] == '\0') {
                    //if check for '\0' then return pointer of it
                    //then non found
                    return (c == '\0') ? (char*)(s + idx) : NULL;
                }

                return (char*)(s + idx);
            }
            s += 16;
        }
    }

    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    return (c == '\0') ? (char*)s : NULL;
}*/

/*
static inline char* NoCRT_strchr_avx2_fast(const char* s, int c) {
    char ch = (char)c;
    __m256i pattern = _mm256_set1_epi8(ch);
    __m256i zero = _mm256_setzero_si256();

    while (1) {
        __m256i v = _mm256_loadu_si256((const __m256i*)s);
        __m256i match = _mm256_cmpeq_epi8(v, pattern);
        __m256i nulls = _mm256_cmpeq_epi8(v, zero);

        //get 32bit masks and make them to 64bit registers CPU
        uint64_t mask_match = (uint32_t)_mm256_movemask_epi8(match);
        uint64_t mask_null  = (uint32_t)_mm256_movemask_epi8(nulls);

        if (mask_match | mask_null) {
            if (ch == '\0') {
                unsigned long idx;
                _BitScanForward64(&idx, mask_null);
                return (char*)(s + idx);
            }

            //bit trick (mask & -mask) isolate the smallest bit
            // checking these things tells to cpu what index is smoller
            //without heaveh instructions
            if (mask_match && ((mask_match & -mask_match) <= (mask_null & -mask_null))) {
                unsigned long idx;
                _BitScanForward64(&idx, mask_match);
                return (char*)(s + idx);
            }
            return NULL;
        }
        s += 32;
    }
}
*/
//might read uhh after the page is done
static inline char* NoCRT_strchr(const char* s, int c) {
    char ch = (char)c;
    __m128i pattern = _mm_set1_epi8(ch);
    __m128i zero = _mm_setzero_si128();

    // safe sheesh for Page Fault bug
    // read byte by byte until address will not align to 16bytes
    while (((uintptr_t)s & 15) != 0) {
        if (*s == ch) return (char*)s;
        if (*s == '\0') return (ch == '\0') ? (char*)s : NULL;
        s++;
    }

    // anotha cycle
    while (1) {
        // FIXED: address is safely aligned
        __m128i v = _mm_loadu_si128((const __m128i*)s); //not fully tested but use
        __m128i match = _mm_cmpeq_epi8(v, pattern);     //_mm_load_si128 instead
        __m128i nulls = _mm_cmpeq_epi8(v, zero);

        unsigned int mask_match = (unsigned int)_mm_movemask_epi8(match);
        unsigned int mask_null = (unsigned int)_mm_movemask_epi8(nulls);

        if (mask_match | mask_null) {
            unsigned long idx_match = 16;
            unsigned long idx_null = 16;

            // find index of da first match idk(0..15)
            if (mask_match) {
                _BitScanForward(&idx_match, mask_match);
            }
            // find index of da first null/zero idk(0..15)
            if (mask_null) {
                _BitScanForward(&idx_null, mask_null);
            }

            // if symbol meeted earlier than 0(or it's da null terminator)
            if (idx_match < idx_null) {
                return (char*)(s + idx_match);
            }

            // if 0 meeted earlier
            if (ch == '\0' && idx_null < 16) {
                return (char*)(s + idx_null);
            }

            //if null terminator meeted earlier than symbol then no matches
            return NULL;
        }
        s += 16;
    }
}

static int NoCRT_vsnprintf(char* buf, NCsize_t size, const char* fmt, va_list args) {
    char* p = buf;
    char* end = buf + size - 1;
    const char* f = fmt;

    while (*f && p < end) {
        if (*f != '%') {
            *p++ = *f++;
            continue;
        }
        f++; /* fuck % */

        if (*f == 's') {
            const char* s = va_arg(args, const char*);
            while (*s && p < end) *p++ = *s++;
            f++;
        }
        else if (*f == 'd' || *f == 'i') {
            int val = va_arg(args, int);
            p += NoCRT_itoa(val, p, 10);
            f++;
        }
        else if (*f == 'c') {
            *p++ = (char)va_arg(args, int);
            f++;
        }
        else if (*f == '%') {
            *p++ = '%';
            f++;
        }
        else if (*f == 'l' && *(f + 1) == 'd') {
            long val = va_arg(args, long);
            NCuint64_t uval = (NCuint64_t)val;
            if (val < 0) {
                if (p < end) *p++ = '-';
                uval = (NCuint64_t)(-((NCint64_t)val)); // Безопасное инвертирование через int64
            }
            /* into the itoa and not the val, and additional uval */
            p += NoCRT_itoa64(uval, p);
            f += 2;
        }
        else {
            *p++ = *f++; /* if unknown then just eat it anyways */
        }
    }
    *p = '\0';
    return (int)(p - buf);
}

/* sprintf */
int NoCRT_sprintf(char* buf, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = NoCRT_vsnprintf(buf, (NCsize_t)-1, fmt, args);
    va_end(args);
    return n;
}

/* snprintf */
int NoCRT_snprintf(char* buf, NCsize_t size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = NoCRT_vsnprintf(buf, size, fmt, args);
    va_end(args);
    return n;
}

int NoCRT_printf(const char* fmt, ...) {
    char buf[4096];
    va_list args;
    va_start(args, fmt);
    int n = NoCRT_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    unsigned long written;
    void* h = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteConsoleA(h, buf, (unsigned long)n, &written, 0);
    return n;
}


//#define NULL 0

extern int NoCRT_cpu_has_avx2(void);

static inline char* NoCRT_strchr_avx2(const char* s, int c) {
    char ch = (char)c;

    //fallback
    if (!NoCRT_cpu_has_avx2()) {
        while (*s) {
            if (*s == ch) return (char*)s;
            s++;
        }
        return (ch == '\0') ? (char*)s : NULL;
    }

    //safe sheesh: align pointer to 32 byte edge
    // obrabotat' this until (s % 32) != 0
    // so vector reading __m256i never goes out of bounds of a page
    while (((uintptr_t)s & 31) != 0) {
        if (*s == ch) return (char*)s;
        if (*s == '\0') return (ch == '\0') ? (char*)s : NULL;
        s++;
    }

    //AVX2
    __m256i pattern = _mm256_set1_epi8(ch);
    __m256i zero = _mm256_setzero_si256();

    while (1) {
        //align reading of 32 byte
        __m256i v = _mm256_load_si256((const __m256i*)s);

        __m256i match = _mm256_cmpeq_epi8(v, pattern);
        __m256i nulls = _mm256_cmpeq_epi8(v, zero);

        //get 32bits masks (bigger bits are saved as 0 in 64bit int)
        NCuint64_t mask_match = (NCuint32_t)_mm256_movemask_epi8(match);
        NCuint64_t mask_null = (NCuint32_t)_mm256_movemask_epi8(nulls);

        if (mask_match || mask_null) {
            unsigned long idx_match = 32;
            unsigned long idx_null = 32;

            if (mask_match) _BitScanForward64(&idx_match, mask_match);
            if (mask_null)  _BitScanForward64(&idx_null, mask_null);

            // if finding value goes after 0(or matches with him '\0')
            if (idx_match <= idx_null) {
                return (char*)(s + idx_match);
            }

            //if null terminator was meeted earlier then it's idk fine?
            return NULL;
        }
        s += 32;
    }
}

//HASH — CRC32 INTRINSIC
static inline NCuint32_t NoCRT_hash32(const void* data, NCsize_t len) {
    NCuint32_t hash = 0;
    const unsigned char* p = (const unsigned char*)data;
    NCsize_t i = 0;

    //main cycle to 4bytes
    for (; i + 4 <= len; i += 4) {
        hash = _mm_crc32_u32(hash, *(const NCuint32_t*)(p + i));
    }

    //tail
    if (i + 2 <= len) {
        hash = _mm_crc32_u16(hash, *(const NCuint16_t*)(p + i));
        i += 2;
    }
    if (i < len) {
        hash = _mm_crc32_u8(hash, p[i]);
    }

    return hash;
}

//lining cache with CRC32
static inline NCuint32_t NoCRT_hash(const char* s) {
    NCuint32_t hash = 0;
    while (*s) {
        hash = _mm_crc32_u8(hash, (unsigned char)*s++);
    }
    return hash;
}
/*
#pragma once
#include <immintrin.h> //for lazy sheesh

// 64bit Thread-Local generator sheesh
static thread_local uint64_t NoCRT_rng_state = 0;

static inline int NoCRT_rand(void) {
    //lazy init: if stream is new and new seed is 0
    if (NoCRT_rng_state == 0) {
        //get true apparat entropy seed through RDRAND
        if (!_rdrand64_step(&NoCRT_rng_state)) {
            //if RDRAND sucks then get counter of CPU tacts as fallback
            NoCRT_rng_state = __rdtsc() | 1;
        }
    }

    //WyRand/SplitMix64
    NoCRT_rng_state += 0x9E3779B97F4A7C15ULL;

    //MSVC x64 does things with 64x64 and offset into one MUL instruction as i know
    uint64_t z = NoCRT_rng_state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    uint64_t result = z ^ (z >> 31);

    //return cool 31 bit int (just like rand)
    return (int)(result & 0x7FFFFFFF);
}
*/
/*//fast boi
#pragma once
#include <immintrin.h>

static thread_local uint32_t NoCRT_rng_state = 0;

static inline int NoCRT_rand(void) {
    // lazeh seed: if 0 then init through rdrand/rdtsc
    if (NoCRT_rng_state == 0) {
        unsigned int seed;
        if (!_rdrand32_step(&seed)) {
            seed = (unsigned int)__rdtsc();
        }
        NoCRT_rng_state = seed | 1; //state doesn't needs to be 0
    }

    //cool Xorshift32
    uint32_t x = NoCRT_rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    NoCRT_rng_state = x;

    return (int)(x & 0x7FFFFFFF);
}
*/
/*static inline int NoCRT_rand_hw(void) {
    int val;
    if (_rdrand32_step((unsigned int*)&val)) {
        return val & 0x7FFFFFFF;
    }
    // fallback
    NoCRT_rand_seed = NoCRT_rand_seed * 1103515245 + 12345;
    return (int)((NoCRT_rand_seed >> 16) & 0x7FFF);
}*/
//RANDOM — RDRAND
// fallaback for every stream (4bytes in sector TLS)
//static thread_local uint32_t NoCRT_rand_seed = 0x1337FFFF;

/*//PCG
#pragma once
#include "NoCRT.h"
#include <immintrin.h>

// 64bit sheesh
static thread_local uint64_t NoCRT_pcg_state = 0;

static inline int NoCRT_rand_strong(void) {
    //lazy init: seed one time in lifetime
    if (NoCRT_pcg_state == 0) {
        // try RDRAND
        if (!_rdrand64_step(&NoCRT_pcg_state)) {
            // if cpu is dying then get cpu tact counter as fallback
            NoCRT_pcg_state = __rdtsc();
        }
        //state PCG
        NoCRT_pcg_state |= 1ULL;
    }

    //PCG32
    //6364136223846793005ULL magiek
    uint64_t oldstate = NoCRT_pcg_state;
    NoCRT_pcg_state = oldstate * 6364136223846793005ULL + 1ULL;

    //mix bits (XSH-RR - xorshift + random rotate)
    //this turns standard LCG into magek
    uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = (uint32_t)(oldstate >> 59u);
    uint32_t result = (xorshifted >> rot) | (xorshifted << ((-rot) & 31));

    //return sheesh from 0 to 2 147 483 647
    return (int)(result & 0x7FFFFFFF);
}
*/

//ATOMIC OPERATIONS
static inline NCint32_t NoCRT_atomic_add(volatile NCint32_t* ptr, NCint32_t val) {
    return _InterlockedExchangeAdd((volatile long*)ptr, val);
}

static inline NCint64_t NoCRT_atomic_add64(volatile NCint64_t* ptr, NCint64_t val) {
    return _InterlockedExchangeAdd64(ptr, val);
}

static inline NCint32_t NoCRT_atomic_cas(volatile NCint32_t* ptr, NCint32_t expected, NCint32_t desired) {
    return _InterlockedCompareExchange((volatile long*)ptr, desired, expected);
}

static inline void* NoCRT_atomic_cas_ptr(void* volatile* ptr, void* expected, void* desired) {
    return _InterlockedCompareExchangePointer(ptr, desired, expected);
}

//FAST MATH INTRINSICS
static inline float NoCRT_rsqrt(float x) {
    __m128 v = _mm_set_ss(x);
    v = _mm_rsqrt_ss(v);
    //one iteration of newton per iteration
    __m128 half = _mm_set_ss(0.5f);
    __m128 orig = _mm_set_ss(x);
    __m128 three = _mm_set_ss(1.5f);
    v = _mm_mul_ss(v, _mm_sub_ss(three, _mm_mul_ss(_mm_mul_ss(orig, half), _mm_mul_ss(v, v))));
    return _mm_cvtss_f32(v);
}

static inline float NoCRT_rcp(float x) {
    __m128 v = _mm_set_ss(x);
    v = _mm_rcp_ss(v);
    return _mm_cvtss_f32(v);
}

//floor/ceil
static inline int NoCRT_floor_to_int(float x) {
    return _mm_cvtt_ss2si(_mm_set_ss(x));
}

static inline int NoCRT_round_to_int(float x) {
    //_mm_cvtss_si32 round-to-nearest-even
    return _mm_cvtss_si32(_mm_set_ss(x));
}

//BIT SCAN
/*static inline int NoCRT_clz32(uint32_t x) {
    if (!x) return 32;
    unsigned long idx;
    _BitScanReverse(&idx, x);
    return 31 - idx;
}*/
static inline int NoCRT_clz32(NCuint32_t x) {
    if (!x) return 32;

    //if cpu supports cooler instructions (well it goes with AVX2)
    //_lzcnt_u32 compiles into LZCNT
    if (NoCRT_cpu_avx2) {
        return (int)_lzcnt_u32(x);
    }

    //fallback for old CPUS
    unsigned long idx;
    _BitScanReverse(&idx, x);
    return 31 - (int)idx;
}

static inline int NoCRT_ctz32(NCuint32_t x) {
    if (!x) return 32;
    unsigned long idx;
    _BitScanForward(&idx, x);
    return idx;
}

/*static inline int NoCRT_popcount32(uint32_t x) {
    return (int)__popcnt(x);
}*/
static inline int NoCRT_popcount32(NCuint32_t x) {
    // if cpu supports SSE4.2+, then POPCNT is safe
    //check through sse2/avx2 or NoCRT_cpu_sse42 because of cascade shet
    if (NoCRT_cpu_avx2) {
        return (int)__popcnt(x);
    }

    // fallback (hamming's algorithm/SWAR)
    // https://en.wikipedia.org/wiki/Hamming_code
    // works without dividing and other sheesh for 12tacts of cpu
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x + (x >> 4)) & 0x0F0F0F0F;
    x = x + (x >> 8);
    x = x + (x >> 16);
    return (int)(x & 0x0000003F);
}

static inline NCuint32_t NoCRT_bswap32(NCuint32_t x) {
    return _byteswap_ulong(x);
}

//PREFETCH
static inline void NoCRT_prefetch_read(const void* ptr) {
    _mm_prefetch((const char*)ptr, _MM_HINT_T0);
}

static inline void NoCRT_prefetch_write(void* ptr) {
    _m_prefetchw(ptr);
}

static inline void NoCRT_prefetch_l1(const void* ptr) {
    _mm_prefetch((const char*)ptr, _MM_HINT_T1);
}

static inline void NoCRT_prefetch_l2(const void* ptr) {
    _mm_prefetch((const char*)ptr, _MM_HINT_T2);
}

//PAUSE / YIELD (for spinlock)
static inline void NoCRT_pause(void) {
    _mm_pause();
}

static inline void NoCRT_mfence(void) {
    _mm_mfence();
}

//SIMD
#define NOCRT_MM_SET1_EPI8(x)  _mm_set1_epi8((char)(x))
#define NOCRT_MM_SET1_EPI32(x) _mm_set1_epi32((int)(x))
#define NOCRT_MM_ZERO           _mm_setzero_si128()

//MEMORY
/*static inline void NoCRT_memset(void* ptr, int value, NCsize_t num) {
    unsigned char* p = (unsigned char*)ptr;
    while (num--) *p++ = (unsigned char)value;
}*/

/*static inline void NoCRT_memcpy(void* dst, const void* src, size_t num) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    //copy to 8byte if aligned
    if ((((uintptr_t)d | (uintptr_t)s) & 7) == 0) {
        uint64_t* d64 = (uint64_t*)d;
        const uint64_t* s64 = (const uint64_t*)s;
        size_t n = num >> 3;
        while (n--) *d64++ = *s64++;
        num &= 7;
        d = (unsigned char*)d64;
        s = (const unsigned char*)s64;
    }
    while (num--) *d++ = *s++;
}*/

/*static inline int NoCRT_memcmp(const void* a, const void* b, size_t num) {
    const unsigned char* p1 = (const unsigned char*)a;
    const unsigned char* p2 = (const unsigned char*)b;
    while (num-- && *p1 == *p2) { p1++; p2++; }
    return (num == (NCsize_t)-1) ? 0 : *p1 - *p2;
}*/

//STRINGS
/*static inline size_t NoCRT_strlen(const char* s) {
    const char* p = s;
    while (*p) p++;
    return p - s;
}*/

/*static inline int NoCRT_strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *(unsigned char*)a - *(unsigned char*)b;
}*/

static inline int NoCRT_strncmp(const char* a, const char* b, NCsize_t n) {
    if (!n) return 0;

    NoCRT_init_cpu_features();

    if (NoCRT_cpu_sse2 && n >= 16) {
        __m128i zero = _mm_setzero_si128();

        while (n >= 16) {
            __m128i va = _mm_loadu_si128((const __m128i*)a);
            __m128i vb = _mm_loadu_si128((const __m128i*)b);

            __m128i cmp_eq = _mm_cmpeq_epi8(va, vb);
            int mask_eq = _mm_movemask_epi8(cmp_eq);

            //check if null
            __m128i null_a = _mm_cmpeq_epi8(va, zero);
            __m128i null_b = _mm_cmpeq_epi8(vb, zero);
            __m128i null_any = _mm_or_si128(null_a, null_b);
            int mask_null = _mm_movemask_epi8(null_any);

            //bits null or not (in n bounds)
            __m128i not_equal = _mm_andnot_si128(cmp_eq, _mm_set1_epi8(-1));
            __m128i diff_or_null = _mm_or_si128(not_equal, null_any);
            int mask = _mm_movemask_epi8(diff_or_null);

            if (mask) {
                unsigned long idx;
                _BitScanForward(&idx, (unsigned long)mask);

                //if pos is out of bounds n, then align with n
                if ((NCsize_t)idx >= n) return 0;

                return (unsigned char)a[idx] - (unsigned char)b[idx];
            }

            a += 16;
            b += 16;
            n -= 16;
        }
    }

    //fallback
    while (n && *a && *a == *b) { a++; b++; n--; }
    return (n == 0) ? 0 : *(unsigned char*)a - *(unsigned char*)b;
}

/*static inline char* NoCRT_strcpy(char* dst, const char* src) {
    char* d = dst;
    while (*src) *d++ = *src++;
    *d = '\0';
    return dst;
}*/

/*static inline char* NoCRT_strncpy(char* dst, const char* src, NCsize_t n) {
    char* d = dst;
    while (n && *src) { *d++ = *src++; n--; }
    while (n--) *d++ = '\0';
    return dst;
}*/

static inline char* NoCRT_strcat(char* dst, const char* src) {
    NoCRT_strcpy(dst + NoCRT_strlen(dst), src);
    return dst;
}

/*static inline char *nocrt_strchr(const char *str, int character)
{
    while(*str != '\0')
    {
        if(*str == character)
        {
            return (char *)str;
        }
        str++;
    }

    if(character == '\0')
    {
        return (char *)str;
    }

    return NULL;
}
}*/

/*static inline char* NoCRT_strrchr(const char* s, int c) {
    char* r = NULL;
    while (*s) {
        if (*s == (char)c) r = (char*)s;
        s++;
    }
    if (c == '\0') return (char*)s;  // return pointer to null terminator
    return r;
}*/

/*
use pcmpistri for 2-4bytes = 1 instruction per 16bytes
for long KMP
same fog Agner
*/
static inline char* NoCRT_strstr(const char* haystack, const char* needle) {
    NCsize_t nlen = NoCRT_strlen(needle);
    if (!nlen) return (char*)haystack;

    NoCRT_init_cpu_features();

    if (NoCRT_cpu_sse2 && nlen >= 2) {
        //FIXED: safe byte alignment haystack until 16byte
        // to stop fucking with Access Violation on the edges of da pages
        while (((uintptr_t)haystack & 15) != 0) {
            if (*haystack == '\0') return NULL;
            if (*haystack == needle[0] && NoCRT_strncmp(haystack, needle, nlen) == 0) {
                return (char*)haystack;
            }
            haystack++;
        }

        __m128i first_char = _mm_set1_epi8(needle[0]);
        __m128i zero = _mm_setzero_si128();

        while (1) {
            __m128i v = _mm_load_si128((const __m128i*)haystack);

            __m128i nulls = _mm_cmpeq_epi8(v, zero);
            int null_mask = _mm_movemask_epi8(nulls);

            __m128i matches = _mm_cmpeq_epi8(v, first_char);
            int match_mask = _mm_movemask_epi8(matches);

            if (null_mask) {
                unsigned long null_pos;
                _BitScanForward(&null_pos, (unsigned long)null_mask);
                //safe cutting of matches after NULL through 64bit cast
                match_mask &= (int)((1ULL << null_pos) - 1);
            }

            while (match_mask) {
                unsigned long idx;
                _BitScanForward(&idx, (unsigned long)match_mask);

                if (NoCRT_strncmp(haystack + idx, needle, nlen) == 0) {
                    return (char*)(haystack + idx);
                }

                match_mask &= match_mask - 1;
            }

            if (null_mask) {
                return NULL;
            }

            haystack += 16;
        }
    }

    //fallaback
    for (; *haystack; haystack++)
        if (*haystack == *needle && NoCRT_strncmp(haystack, needle, nlen) == 0)
            return (char*)haystack;
    return NULL;
}


//CONVERSION
static inline int NoCRT_atoi(const char* s) {
    int n = 0, sign = 1;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') { n = n * 10 + (*s - '0'); s++; }
    return n * sign;
}

static inline float NoCRT_atof(const char* s) {
    float n = 0.0f, sign = 1.0f, frac = 0.1f;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') { sign = -1.0f; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') { n = n * 10.0f + (*s - '0'); s++; }
    if (*s == '.') {
        s++;
        while (*s >= '0' && *s <= '9') { n += (*s - '0') * frac; frac *= 0.1f; s++; }
    }
    return n * sign;
}

static inline long NoCRT_strtol(const char* s, char** endptr, int base) {
    unsigned long n = 0;
    int sign = 1;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') s++;
    if (base == 0) base = (*s == '0') ? ((s[1] == 'x' || s[1] == 'X') ? 16 : 8) : 10;
    if (base == 16 && *s == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    while (1) {
        int d;
        if (*s >= '0' && *s <= '9') d = *s - '0';
        else if (*s >= 'a' && *s <= 'z') d = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'Z') d = *s - 'A' + 10;
        else break;
        if (d >= base) break;
        n = n * base + d;
        s++;
    }
    if (endptr) *endptr = (char*)s;
    return (long)(sign > 0 ? n : ~n + 1);
}

//RANDOM
static NCuint32_t NoCRT_rand_seed = 12345;

static inline void NoCRT_srand(NCuint32_t seed) { NoCRT_rand_seed = seed; }

static inline int NoCRT_rand(void) {
    NoCRT_rand_seed = NoCRT_rand_seed * 1103515245 + 12345;
    return (int)((NoCRT_rand_seed >> 16) & 0x7FFF);
}

static inline float NoCRT_randf(float min, float max) {
    return min + (float)NoCRT_rand() / 32767.0f * (max - min);
}

//HASH
/*static inline uint32_t NoCRT_hash(const char* s) {
    uint32_t h = 5381;
    while (*s) h = ((h << 5) + h) + (unsigned char)*s++;
    return h;
}*/

/*typedef void (*NoCRT_memset_t)(void*, int, NCsize_t);
typedef void (*NoCRT_memcpy_t)(void*, const void*, NCsize_t);
typedef void (*NoCRT_memmove_t)(void*, const void*, NCsize_t);

static NoCRT_memset_t  NoCRT_memset_override = NULL;
static NoCRT_memcpy_t  NoCRT_memcpy_override = NULL;
static NoCRT_memmove_t NoCRT_memmove_override = NULL;

#define NOCRT_OVERRIDE_MEMSET(fn)  (NoCRT_memset_override  = (fn))
#define NOCRT_OVERRIDE_MEMCPY(fn)  (NoCRT_memcpy_override  = (fn))
#define NOCRT_OVERRIDE_MEMMOVE(fn) (NoCRT_memmove_override = (fn))*/

//MIN/MAX/CLAMP
#define NOCRT_MIN(a,b)  (((a) < (b)) ? (a) : (b))
#define NOCRT_MAX(a,b)  (((a) > (b)) ? (a) : (b))
#define NOCRT_CLAMP(x,a,b) NOCRT_MIN(NOCRT_MAX((x),(a)),(b))
#define NOCRT_ZERO(s)  NoCRT_memset(&(s), 0, sizeof(s))


/*
TO USE

//bad can fall on AVX2, if line is on the end of da page
const char* str = "Hello";

//good dr freeman (buffer magek with a little bit of vector)
char my_str[64] = "Hello"; //cpu can read safely nulls in the end of da buffer

*/
/*
to port this to GCC LINUX

just change intrinsics names
about rep stosb just use inline assembler and with other just change names
*/
/*
MAYBE TODO

change NoCRT_init_cpu_features() to inline funcs
like:

// in the beginning of every big and hard func
if (NoCRT_cpu_features_cached != 2) {
    NoCRT_init_cpu_features_slow();
}
// then use NoCRT_cpu_avx2 without any other checks
compiler will likely change this if into "cmp [cache], 2 + jne slow"
4-5 tacts smaller per call


add _nt for memcpy and other funcs
*/
/*
if you will use just int main(){} then CRT will connect bcuz this is shit, so make your own entry shit
*/
