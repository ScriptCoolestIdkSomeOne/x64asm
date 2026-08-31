# Why?
because u need to know how to use it, geez

## How?
okay now to the cool things

<details>

<summary>raw method</summary>
idk why but without this huge spaces .md will not just work
```

    #include "NoCRT.h" // FOR NoCRT_printf, OR JUST USE <stdio.h>
    
    #include "x64_asm.h" //MAIN HEADER
    
    /* START OF THE PROGRAM */
    
    int main() {

    /* INIT JIT MEMORY */
    // JitMemory_init() inits global memory manager that creates:
    
    //code_arena(arena for RWX/RX), data_arena(arena for data), temp_stack(stack for temp buffers), f16_vec_pool(pool for float16)
    
    //returns 0 on error and 1 on success
    
    if (!JitMemory_init()) {
    
        NoCRT_printf("JIT INIT ERROR: can't init memory\n");
        
        return 1; // exit with error
        
    }
    
    /* EMITTER CREATION */

    // X64Assembler structure that has:
    
    //base (emitter where the bytes are writed), labels(shitty shit), base.cur is position pointer, base.end - base.start is the size
    
    X64Assembler a; // emitter on the stack
    
    // A_init(&a) inits emitter:
    // inits memort through JIT_ALLOC_CODE
    //nulls counters
    // gets structure ready to work
    A_init(&a);
    
    /* GENERATION OF THE MACHINE CODE */
    // 'A_mov_r_imm(&a, REG_RAX, 42)' generates instructions:
    //mov rax, 42
    //puts 42 into the RAX register (contains result in there)
    //parameters:
    // &a is pointer to the register
    //REG_RAX register collecter (from x64_registers.h)
    //42 number(immediate)
    A_mov_r_imm(&a, REG_RAX, 42);
    
    // A_ret(&a) generates ret:
    //returns control of the calling func
    // stack: (RSP) -> RIP (address of returning), RAX should have the result
    A_ret(&a);
    /* FINALIZATION AND RUNNING */
    // A_run(&a) ends code generation, it does:
    // adds ret, if there is no ret, resets cache of the instructions(FlushInstructionCache), changes permissions from RW to RX (otherwise an error)
    // and then runs the code(calls generated func)
    A_run(&a);
    /* CALLING GENERATED FUNC */
    //a.base.start is a pointer on the generated code
    //this memory can be used with pointer on the func
    //int (*func)(void) point that doesn't accept arguments and returns int
    int (*func)(void) = (int(*)(void))a.base.start;
    
    //call generated func, it will return 42 (well because of mov rax, 42)
    int result = func();
    
    NoCRT_printf("result: %d\n", result);//should be "result: 42"
    
    /* DELETING SHIT */
    // JitMemory_shutdown() frees all of memory:
    /* deletes: code_arena (VirtualFree), data_arena, temp_stack, f16_vec_pool
    nulls glob structs
    after this new initialization can be called */
    JitMemory_shutdown();

    /* END OF THE PROGRAM */
    return 0;
}
}```
</details>

<details>
<summary>main __asm method</summary>

```
#include "NoCRT.h" // FOR NoCRT_printf, OR JUST USE <stdio.h>

#include "x64_asm.h" // MAIN HEADER

int main() {

    // jit init

    if (!JitMemory_init()) {

        NoCRT_printf("JIT INIT ERROR\n");

        return 1;

    }

    // generation through __asm (basically parser, you also can use it as blocks of code too)
    void* func; // pointer on generated func

    // __asm("assembler", &pointer)
    //here we do the inline thingy
    __asm("mov rax, 42; ret", func); //don't use & because macro sucks
    /* just makes this line: __asm("mov rax, 42; ret", &func); into
    do {
    // ...
    (&func) = (void*)__a.base.start;
    } while(0); */

    //calling generated sheesh
    int result = ((int(*)())func)(); //i forgot how this works

    //result
    NoCRT_printf("Result: %d\n", result);   // 42

    // memory clearing
    JitMemory_shutdown();

    return 0;
}
}```
</details>
