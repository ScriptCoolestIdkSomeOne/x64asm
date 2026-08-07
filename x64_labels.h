/*
* well labels!11!!1
*/
#pragma once
#include <stdint.h>
#include "x64_registers.h"
//#include "x64_asm.h"

static uint32_t nothash_str(const char* s) {
    uint32_t h = 5381;
    while (*s) h = ((h << 5) + h) + *s++;
    return h;
}

typedef struct {
    uint32_t hash;
    int32_t offset;
    int32_t pp[32], ie[32];
    uint8_t pc;
    uint8_t is[32];
} Label;

typedef struct { Label labels[256]; uint8_t count; } X64Labels;

static void L_init(X64Labels* l) { l->count = 0; }

static int L_find(X64Labels* l, const char* name) {
    uint32_t h = nothash_str(name);
    for (int i = 0; i < l->count; i++) if (l->labels[i].hash == h) return i;
    return -1;
}

static void L_bind(X64Labels* l, const char* name, size_t pos) {
    int i = L_find(l, name);
    if (i < 0) { if (l->count >= 255) return; i = l->count++; l->labels[i].hash = nothash_str(name); l->labels[i].pc = 0; }
    l->labels[i].offset = (int32_t)pos;
}

static int32_t L_get(X64Labels* l, const char* name) {
    int i = L_find(l, name);
    return (i >= 0 && l->labels[i].offset >= 0) ? l->labels[i].offset : -1;
}

static void L_add(X64Labels* l, const char* name, size_t pp, size_t ie, int s) {
    int i = L_find(l, name);
    if (i < 0) { if (l->count >= 255) return; i = l->count++; l->labels[i].hash = nothash_str(name); l->labels[i].offset = -1; l->labels[i].pc = 0; }
    if (l->labels[i].pc < 32) {
        uint8_t n = l->labels[i].pc;
        l->labels[i].pp[n] = (int32_t)pp;
        l->labels[i].ie[n] = (int32_t)ie;
        l->labels[i].is[n] = s ? 1 : 0;
        l->labels[i].pc++;
    }
}

#define L_resolve(l, e, cp) do { \
    for (int _i=0; _i<(l)->count; _i++) { \
        if ((l)->labels[_i].offset < 0) continue; \
        for (int _j=0; _j<(l)->labels[_i].pc; _j++) { \
            int32_t _d = (l)->labels[_i].offset - (l)->labels[_i].ie[_j]; \
            if ((l)->labels[_i].is[_j]) E_setb(e, (l)->labels[_i].pp[_j], (uint8_t)(_d&0xFF)); \
            else E_setd(e, (l)->labels[_i].pp[_j], _d); \
        } \
        (l)->labels[_i].pc = 0; \
    } \
} while(0)

static void L_reset(X64Labels* l) { l->count = 0; }