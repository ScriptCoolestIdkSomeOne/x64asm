#pragma once
#include "x64_core.h"
#include "x64_memory.h"
#include "x64_labels.h"

template<typename T>
struct Optional {
    T value;
    bool has;
    Optional() : has(false) {}
    Optional(T v) : value(v), has(true) {}
    T operator*() { return value; }
    bool has_value() { return has; }
};

class X64Assembler : public X64Emitter {
    X64Labels labels_;

public:
    static constexpr Reg64 rax = Reg64::RAX, rcx = Reg64::RCX, rdx = Reg64::RDX, rbx = Reg64::RBX;
    static constexpr Reg64 rsp = Reg64::RSP, rbp = Reg64::RBP, rsi = Reg64::RSI, rdi = Reg64::RDI;
    static constexpr Reg64 r8 = Reg64::R8, r9 = Reg64::R9, r10 = Reg64::R10, r11 = Reg64::R11;
    static constexpr Reg64 r12 = Reg64::R12, r13 = Reg64::R13, r14 = Reg64::R14, r15 = Reg64::R15;

    X64Assembler() : X64Emitter() {
        begin();
    }

    void mov(Reg64 dst, uint64_t imm) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(dst)) rex |= REX_B;
        emit(rex);
        emit(0xB8 + (reg_index(dst) & 0x7));
        emit_qword(imm);
    }

    void mov(Reg64 dst, Reg64 src) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(dst)) rex |= REX_B;
        if (reg_needs_rex(src)) rex |= REX_R;
        emit(rex);
        emit(0x89);
        emit_modrm(0x3, reg_index(src) & 0x7, reg_index(dst) & 0x7);
    }

    void add(Reg64 dst, int imm) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(dst)) rex |= REX_B;
        emit(rex);
        
        //choose the shortest or longest opcode, depends on the number magic
        if (imm >= -128 && imm <= 127) {
            emit(0x83); // ADD r/m64, imm8
            emit_modrm(0x3, 0, reg_index(dst) & 0x7);
            emit((uint8_t)imm);
        } else {
            emit(0x81); //ADD r/m64, imm32
            emit_modrm(0x3, 0, reg_index(dst) & 0x7);
            emit_dword((uint32_t)imm);
        }
    }
    void add(Reg64 dst, Reg64 src) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(dst)) rex |= REX_B;
        if (reg_needs_rex(src)) rex |= REX_R;
        emit(rex);
        emit(0x01);
        emit_modrm(0x3, reg_index(src) & 0x7, reg_index(dst) & 0x7);
    }
    void ret() { emit(OP_RET); }
    void nop() { emit(OP_NOP); }
    void int3() { emit(OP_INT3); }

    void bind(const char* name) {
        labels_.bind(name, current_pos());
    }

    void jmp(const char* label) {
    int32_t offset = labels_.get_offset(label);
    if (offset >= 0) {
        int32_t disp = offset - ((int32_t)current_pos() + 5);
        emit(0xE9);
        emit_dword(disp);
    }
    else {
        emit(0xE9);
        // current_pos() this is for dword
        // current_pos() + 4 end of instruction jmp
        labels_.add_pending(label, current_pos(), current_pos() + 4, false);
        emit_dword(0);
    }
}


    void xor_(Reg64 dst, Reg64 src) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(dst)) rex |= REX_B;
        if (reg_needs_rex(src)) rex |= REX_R;
        emit(rex);
        emit(0x31);  //XOR r/m64, r64
        emit_modrm(0x3, reg_index(src) & 0x7, reg_index(dst) & 0x7);
    }
    void sub(Reg64 dst, int imm) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(dst)) rex |= REX_B;
        emit(rex);
        
        // „@„~„p„|„€„s„y„‰„~„€ „t„|„‘ SUB
        if (imm >= -128 && imm <= 127) {
            emit(0x83); // SUB r/m64, imm8
            emit_modrm(0x3, 5, reg_index(dst) & 0x7);
            emit((uint8_t)imm);
        } else {
            emit(0x81); // SUB r/m64, imm32
            emit_modrm(0x3, 5, reg_index(dst) & 0x7);
            emit_dword((uint32_t)imm);
        }
    }

    void sub(Reg64 dst, Reg64 src) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(dst)) rex |= REX_B;
        if (reg_needs_rex(src)) rex |= REX_R;
        emit(rex);
        emit(0x29);  //SUB r/m64, r64
        emit_modrm(0x3, reg_index(src) & 0x7, reg_index(dst) & 0x7);
    }

    void cmp(Reg64 a, Reg64 b) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(a)) rex |= REX_B;
        if (reg_needs_rex(b)) rex |= REX_R;
        emit(rex);
        emit(0x39);  //CMP r/m64, r64
        emit_modrm(0x3, reg_index(b) & 0x7, reg_index(a) & 0x7);
    }
    void push(Reg64 reg) {
        uint8_t rex = 0;
        if (reg_needs_rex(reg)) rex = REX_B;
        if (rex) emit(rex);
        emit(0x50 + (reg_index(reg) & 0x7));  // PUSH r64
    }

    void pop(Reg64 reg) {
        uint8_t rex = 0;
        if (reg_needs_rex(reg)) rex = REX_B;
        if (rex) emit(rex);
        emit(0x58 + (reg_index(reg) & 0x7));  //POP r64
    }

    void inc(Reg64 reg) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(reg)) rex |= REX_B;
        emit(rex);
        emit(0xFF);  //INC r/m64
        emit_modrm(0x3, 0, reg_index(reg) & 0x7);
    }

    void dec(Reg64 reg) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(reg)) rex |= REX_B;
        emit(rex);
        emit(0xFF);  //DEC r/m64
        emit_modrm(0x3, 1, reg_index(reg) & 0x7);
    }

    void mul(Reg64 src) {
        //RAX = RAX * src, RDX:RAX
        uint8_t rex = REX_W;
        if (reg_needs_rex(src)) rex |= REX_B;
        emit(rex);
        emit(0xF7);  // MUL r/m64
        emit_modrm(0x3, 4, reg_index(src) & 0x7);
    }

    void imul(Reg64 dst, Reg64 src) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(dst)) rex |= REX_B;
        if (reg_needs_rex(src)) rex |= REX_R;
        emit(rex);
        emit(0x0F);
        emit(0xAF);  //IMUL r64, r/m64
        emit_modrm(0x3, reg_index(dst) & 0x7, reg_index(src) & 0x7);
    }

    void and_(Reg64 dst, Reg64 src) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(dst)) rex |= REX_B;
        if (reg_needs_rex(src)) rex |= REX_R;
        emit(rex);
        emit(0x21);  //AND r/m64, r64
        emit_modrm(0x3, reg_index(src) & 0x7, reg_index(dst) & 0x7);
    }

    void or_(Reg64 dst, Reg64 src) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(dst)) rex |= REX_B;
        if (reg_needs_rex(src)) rex |= REX_R;
        emit(rex);
        emit(0x09);  // OR r/m64, r64
        emit_modrm(0x3, reg_index(src) & 0x7, reg_index(dst) & 0x7);
    }

    void not_(Reg64 reg) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(reg)) rex |= REX_B;
        emit(rex);
        emit(0xF7);  //NOT r/m64
        emit_modrm(0x3, 2, reg_index(reg) & 0x7);
    }

    void neg(Reg64 reg) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(reg)) rex |= REX_B;
        emit(rex);
        emit(0xF7);  // NEG r/m64
        emit_modrm(0x3, 3, reg_index(reg) & 0x7);
    }
    void shl(Reg64 reg, uint8_t count) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(reg)) rex |= REX_B;
        emit(rex);
        if (count == 1) {
            emit(0xD1);  //SHL r/m64, 1
            emit_modrm(0x3, 4, reg_index(reg) & 0x7);
        }
        else {
            emit(0xC1);  //SHL r/m64, imm8
            emit_modrm(0x3, 4, reg_index(reg) & 0x7);
            emit(count);
        }
    }

    void shr(Reg64 reg, uint8_t count) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(reg)) rex |= REX_B;
        emit(rex);
        if (count == 1) {
            emit(0xD1);  //SHR r/m64, 1
            emit_modrm(0x3, 5, reg_index(reg) & 0x7);
        }
        else {
            emit(0xC1);  //SHR r/m64, imm8
            emit_modrm(0x3, 5, reg_index(reg) & 0x7);
            emit(count);
        }
    }

    void movsx(Reg64 dst, Reg32 src) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(dst)) rex |= REX_B;
        if (reg_needs_rex(src)) rex |= REX_R;
        emit(rex);
        emit(0x63);  //MOVSXD r64, r/m32
        emit_modrm(0x3, reg_index(dst) & 0x7, reg_index(src) & 0x7);
    }

    void lea(Reg64 dst, Reg64 base, Reg64 index, uint8_t scale, int32_t disp) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(dst)) rex |= REX_R;
        if (reg_needs_rex(base)) rex |= REX_B;
        if (reg_needs_rex(index)) rex |= REX_X;
        emit(rex);
        emit(0x8D);
        if (disp == 0 && reg_index(base) != 5) {
            emit_modrm(0x0, reg_index(dst) & 0x7, 0x04);
            emit_sib(scale, reg_index(index) & 0x7, reg_index(base) & 0x7);
        }
        else if (disp >= -128 && disp <= 127) {
            emit_modrm(0x1, reg_index(dst) & 0x7, 0x04);
            emit_sib(scale, reg_index(index) & 0x7, reg_index(base) & 0x7);
            emit((uint8_t)disp);
        }
        else {
            emit_modrm(0x2, reg_index(dst) & 0x7, 0x04);
            emit_sib(scale, reg_index(index) & 0x7, reg_index(base) & 0x7);
            emit_dword(disp);
        }
    }

    void test(Reg64 a, Reg64 b) {
        uint8_t rex = REX_W;
        if (reg_needs_rex(a)) rex |= REX_B;
        if (reg_needs_rex(b)) rex |= REX_R;
        emit(rex);
        emit(0x85);  //TEST r/m64, r64
        emit_modrm(0x3, reg_index(b) & 0x7, reg_index(a) & 0x7);
    }

    void xchg(Reg64 a, Reg64 b) {
        if (a == b) return; // „H„p„‹„y„„„p: xchg rax, rax ? „„„„€ NOP, „y„s„~„€„‚„y„‚„…„u„}

        if (reg_index(a) == 0) {
            uint8_t rex = REX_W;
            if (reg_needs_rex(b)) rex |= REX_B;
            emit(rex);
            emit(0x90 + (reg_index(b) & 0x7));
        }
        else if (reg_index(b) == 0) {
            uint8_t rex = REX_W;
            if (reg_needs_rex(a)) rex |= REX_B;
            emit(rex);
            emit(0x90 + (reg_index(a) & 0x7));
        }
        else {
            uint8_t rex = REX_W;
            if (reg_needs_rex(a)) rex |= REX_B;
            if (reg_needs_rex(b)) rex |= REX_R;
            emit(rex);
            emit(0x87);
            emit_modrm(0x3, reg_index(b) & 0x7, reg_index(a) & 0x7);
        }
    }

    void call(Reg64 reg) {
        uint8_t rex = 0;
        if (reg_needs_rex(reg)) rex = REX_B;
        if (rex) emit(rex);
        emit(0xFF);  // CALL r/m64
        emit_modrm(0x3, 2, reg_index(reg) & 0x7);
    }

    void jmp_reg(Reg64 reg) {
        uint8_t rex = 0;
        if (reg_needs_rex(reg)) rex = REX_B;
        if (rex) emit(rex);
        emit(0xFF);  // JMP r/m64
        emit_modrm(0x3, 4, reg_index(reg) & 0x7);
    }

    void run() {
        labels_.resolve_all(*this, current_pos());
        X64Emitter::run();
        labels_.reset(); //clean everything for not overlaying another thing on the past __asm
        begin();
    }

    template<typename RetT = int>
    RetT run_ret() {
        labels_.resolve_all(*this, current_pos());
        RetT r = X64Emitter::run_ret<RetT>();
        labels_.reset(); //clear everything for the clear execution
        begin();
        return r;
    }
};