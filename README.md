# x64asm
MSVC X64 only, header files for __asm like in x86 msvc, this is very optimized solution, not fully complete nor perfect, right now made for only demoscenes, can't be used in commercial way right now


## ayo x64 assembler lovers
x64asm sheesh is fully C/C++ compatible(header files made fully with C syntaxis bcuz i wanted to support C)

latest tests 432kb of private working bytes for this code

```#include "x64_skin.h"
#include "x64_core.h"
#include "x64_asm_legacy.h"
#include "memarehallok.h"
//#include <stdio.h>

int main() {
    //init global sheesh
    if (!JitMemory_init()) {
        //printf("ERROR: JitMemory_init failed\n");
        return 1;
    }

    //create emitter
    X64Emitter e;
    E_init(&e);
    //check if memory is allocted
    if (e.start == NULL) {
        //printf("ERROR: E_init failed no memory allocated\n");
        return 1;
    }

    //code
    MOV(rax, 14654654643);
    ADD(rax, 16616313361);
    RET(); // 386806304 bcuz of 32bit limit
    //right answer: 31270968004

    //then run
    int result = E_run_ret_int(&e);
    //printf("result: %d\n", eicar_string, result);
    return 0;
}
```

also ye, there's a lot of new files and custom libraries

> [!IMPORTANT]  
> not everything was tested properly

i'm just tweaking right now a lot of shit to work properly, but standard sheesh is already working

## ABOUT DOCS
ADDINGGUIDE.MD is already updated, also a few of other documentary files are added for those who want to mess with the jit
added a lot of custom libraries and i need to make docs for them too because i forgot to document them and i was focused more on the x64asm
<details>
<summary>code example from the custom lib</summary>

from NoCRT.H file
```
static inline void NoCRT_memset_nt(void* ptr, int value, size_t num) {
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
        size_t n512 = num >> 9;  // how many blocks with 512 byte(8amount*64byte)
        size_t rem = num & 511; //what remains after da big blocks

        if (n512) {
            __m512i v = _mm512_set1_epi8((char)value);
            for (size_t i = 0; i < n512; i++) {
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
        size_t n64 = rem >> 6;
        if (n64) {
            __m512i v = _mm512_set1_epi8((char)value);
            for (size_t i = 0; i < n64; i++) {
                _mm512_stream_si512((__m512i*)p, v);
                p += 64;
            }
        }

        // give tail to stosb
        size_t tail = rem & 63;
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
        size_t n256 = num >> 8;  //blocks to 256 bytes(8amount * 32bytes)
        size_t rem = num & 255;

        if (n256) {
            __m256i v = _mm256_set1_epi8((char)value);
            for (size_t i = 0; i < n256; i++) {
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
        size_t n32 = rem >> 5;
        if (n32) {
            __m256i v = _mm256_set1_epi8((char)value);
            for (size_t i = 0; i < n32; i++) {
                _mm256_stream_si256((__m256i*)p, v);
                p += 32;
            }
        }

        // well tail duh
        size_t tail = rem & 31;
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
        size_t n256 = num >> 8;  // blocks to 256bytes(16amount * 16byte)
        size_t rem = num & 255;

        if (n256) {
            __m128i v = _mm_set1_epi8((char)value);
            for (size_t i = 0; i < n256; i++) {
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
        size_t n16 = rem >> 4;
        if (n16) {
            __m128i v = _mm_set1_epi8((char)value);
            for (size_t i = 0; i < n16; i++) {
                _mm_stream_si128((__m128i*)p, v);
                p += 16;
            }
        }

        //well ye tail
        size_t tail = rem & 15;
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
```
i might just update this function because i think that i can make better sheesh instead of just repeating same code but just tweaking bytes for SSE/AVX and other sheesh

</details>
