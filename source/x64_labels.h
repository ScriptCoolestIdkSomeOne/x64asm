#pragma once
#include <stdint.h>

struct SimpleLabel {
    char name[64];
    int32_t offset;
    int32_t pending_pos[8];  //where you need disp
    int32_t instruction_end[8]; //end of the instrucitons for the right disp
    uint8_t pending_count;
    uint8_t is_short[8];
};

class X64Labels {
    SimpleLabel labels[256];
    uint8_t count;

    int find(const char* name) {
        for (int i = 0; i < count; i++) {
            const char* a = labels[i].name;
            const char* b = name;
            while (*a && *b && *a == *b) { a++; b++; }
            if (*a == *b) return i;
        }
        return -1;
    }
public:
    X64Labels() : count(0) {}

    void bind(const char* name, size_t current_pos) {
        int idx = find(name);
        if (idx < 0) {
            if (count >= 255) return;
            idx = count++;
            int j = 0;
            while (name[j] && j < 63) { labels[idx].name[j] = name[j]; j++; }
            labels[idx].name[j] = '\0';
            labels[idx].pending_count = 0; //fixed: make everything zeros for new execution
        }
        labels[idx].offset = (int32_t)current_pos;
    }

    int32_t get_offset(const char* name) {
        int idx = find(name);
        return (idx >= 0 && labels[idx].offset >= 0) ? labels[idx].offset : -1;
    }

    //fixed: use patch_pos correctly (where to write) and inst_end (end of the instrucitons for delta)
    void add_pending(const char* name, size_t patch_pos, size_t inst_end, bool is_short) {
        int idx = find(name);
        if (idx < 0) {
            if (count >= 255) return;
            idx = count++;
            int j = 0;
            while (name[j] && j < 63) { labels[idx].name[j] = name[j]; j++; }
            labels[idx].name[j] = '\0';
            labels[idx].offset = -1;
            labels[idx].pending_count = 0; // „R„„‚„p„r„|„u„~„€: „s„p„‚„p„~„„„y„‚„€„r„p„~„~„€ „x„p„~„…„|„‘„u„}!
        }
        if (labels[idx].pending_count < 8) {
            uint8_t n = labels[idx].pending_count;
            labels[idx].pending_pos[n] = (int32_t)patch_pos;
            labels[idx].instruction_end[n] = (int32_t)inst_end;
            labels[idx].is_short[n] = is_short ? 1 : 0;
            labels[idx].pending_count++;
        }
    }

    template<typename Emitter>
    void resolve_all(Emitter& emit, size_t current_pos) {
        for (int i = 0; i < count; i++) {
            if (labels[i].offset < 0) continue;

            for (int j = 0; j < labels[i].pending_count; j++) {
                //meth for the end of the instruction
                int32_t disp = labels[i].offset - labels[i].instruction_end[j];

                if (labels[i].is_short[j]) {
                    emit.set_byte(labels[i].pending_pos[j], (uint8_t)(disp & 0xFF));
                }
                else {
                    emit.set_dword(labels[i].pending_pos[j], disp);
                }
            }
            labels[i].pending_count = 0;
        }
    }

    void reset() { count = 0; }
};
