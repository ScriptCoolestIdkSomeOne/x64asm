#include "NoCRT.h"
#include "x64_asm.h"
#ifdef __cplusplus
extern "C" {
#endif
    void __chkstk(void) {
    }
#ifdef __cplusplus
}
#endif

__declspec(dllimport) void __stdcall ExitProcess(unsigned int uExitCode);
X64Assembler a;
void my(void) {
    if (!JitMemory_init()) {
        NoCRT_printf("JIT INIT ERROR\n");
        ExitProcess(1);
    }

    //X64Assembler a;
    A_init(&a);

    A_mov_r_imm(&a, REG_RAX, 42);
    A_ret(&a);

    A_run(&a);

    int (*func)(void) = (int(*)(void))a.base.start;
    int result = func();

    NoCRT_printf("result: %d\n", result);

    JitMemory_shutdown();
    ExitProcess(0);
}
/* program compiles without crt, you need to add /GL-, /NODEFAULTLIB, custom entry, /GS-
* without /O1 optimization: 10kb .exe, with /O1 optimization: 7kb .exe
* without any of the flags and custom entries: 14kb .exe
*/
