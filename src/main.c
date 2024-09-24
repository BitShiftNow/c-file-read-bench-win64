// System includes
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Intrin.h>
#include <stdio.h>

// Danilib includes
#include "danilib\dani_base.h"

#define DANI_LIB_PROFILER_IMPLEMENTATION
#define DANI_PROFILER_STATIC
#define DANI_PROFILER_ENABLED 1
#include "danilib\dani_profiler.h"
#undef DANI_LIB_PROFILER_IMPLEMENTATION

s32 main(s32 argc, s8 **argv) {
    Unused(argc);
    Unused(argv);

    u32 exit_code = 0;

    dani_BeginProfiling();
    
    // TODO: profile code here
    
    dani_EndProfiling();
    dani_PrintProfilingResults();

    return exit_code;
}
