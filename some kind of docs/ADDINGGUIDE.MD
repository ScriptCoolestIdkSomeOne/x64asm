you can use this code like:

```X64Assembler a;
a.mov(a.rax, 42);
a.add(a.rax, 10);
a.ret();
int result = a.run_ret<int>(); //52)```

```__asm_ret(result) {
        MOV(rax, 10);
        MOV(rcx, 42);
        ADD(rax, rcx);
        RET();
    }
}```

```__asm("mov rax, 10");
__asm("add rax, 42");```

```__asm_block {
    MOV(rax, 10);
    MOV(rcx, 42);
    ADD(rax, rcx);
    RET();
}```




IN CODE

//2 types: special and usual
void shl(Reg64 reg, uint8_t count) {
    if (count == 1) {
        //special: opcode D1 without immediate operand
        uint8_t rex = REX_W;
        if (reg_needs_rex(reg)) rex |= REX_B;
        emit(rex);
        emit(0xD1);                          //opcode for offset to 1
        emit_modrm(0x3, 4, reg_index(reg) & 0x7);
        //emit(count) //don't uncomment
    } else {
        //usual
        uint8_t rex = REX_W;
        if (reg_needs_rex(reg)) rex |= REX_B;
        emit(rex);
        emit(0xC1);
        emit_modrm(0x3, 4, reg_index(reg) & 0x7);
        emit(count);
    }
}




types that you can integrade

TYPE 1:

void mov(Reg64 dst, uint64_t imm) {
    //documentation: REX.W + B8+rd io
    uint8_t rex = REX_W;
    if (reg_needs_rex(dst)) rex |= REX_B;
    emit(rex);
    emit(0xB8 + (reg_index(dst) & 0x7));  //opcode + register number
    emit_qword(imm);                        //x64
}

TYPE 2:

void add(Reg64 dst, Reg64 src) {
    //documentation: REX.W + 01 /r
    uint8_t rex = REX_W;
    if (reg_needs_rex(dst)) rex |= REX_B;  //B for dst
    if (reg_needs_rex(src)) rex |= REX_R;  //R for src
    emit(rex);
    emit(0x01);                             //opcode
    emit_modrm(0x3,                        //mod=11 (register)
               reg_index(src) & 0x7,       //reg = src
               reg_index(dst) & 0x7);      //r/m = dst
}

TYPE 3:

void push(Reg64 reg) {
    //documentation: 50+rd
    uint8_t rex = 0;
    if (reg_needs_rex(reg)) rex = REX_B;
    if (rex) emit(rex);
    emit(0x50 + (reg_index(reg) & 0x7));  //opcode + register number
}

TYPE 4:

void mov_mem(Reg64 dst, Reg64 base, int32_t disp) {
    //documentation: REX.W + 8B /r
    uint8_t rex = REX_W;
    if (reg_needs_rex(dst)) rex |= REX_R;
    if (reg_needs_rex(base)) rex |= REX_B;
    emit(rex);
    emit(0x8B);                            //opcode
    emit_modrm(0x2,                        //mod=10 (disp32)
               reg_index(dst) & 0x7,       //reg = dst
               reg_index(base) & 0x7);     //r/m = base
    emit_dword(disp);                      //offset
}

TYPE 5:(hybrid of 1 and 2)

void operation_reg_imm8(Reg64 reg, uint8_t imm, uint8_t opcode, uint8_t digit) {
    uint8_t rex = REX_W;
    if (reg_needs_rex(reg)) rex |= REX_B;
    emit(rex);
    emit(opcode);
    emit_modrm(0x3, digit, reg_index(reg) & 0x7);
    emit(imm);
}
//then it's oneeliner
//void shl(Reg64 reg, uint8_t count) { operation_reg_imm8(reg, count, 0xC1, 4); }


cool algorightm

go to felixcloutier.com/x86/
find opcode (example: "REX.W + 01 /r")
get type:
   /r → 2 registers (ModRM)
   /0../7 → one register + field reg
   +rd → register in opcode
   ib/iw/id → immediate meaning
write emit() like presented in above
test if works
