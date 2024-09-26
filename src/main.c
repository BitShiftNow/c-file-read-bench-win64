// System includes
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Intrin.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

// Danilib includes
#include "danilib\dani_base.h"

#define DANI_LIB_PROFILER_IMPLEMENTATION
#define DANI_PROFILER_STATIC
#define DANI_PROFILER_ENABLE_ALL
#include "danilib\dani_profiler.h"
#undef DANI_LIB_PROFILER_IMPLEMENTATION

// Benchmark types
typedef struct BUFFER buffer;
struct BUFFER {
    u8 *base;
    u64 capacity;
};

typedef struct BENCHMARK_CONTEXT benchmark_context;
struct BENCHMARK_CONTEXT {
    buffer read_buffer;

    u64 file_length;

    const s8 *filename;
};

// Project includes
#include "benchmarks.c"

s32 main(s32 argc, s8 **argv) {
    Unused(argc);
    Unused(argv);

    const u32 iteration_count = 10; // TODO: Read iteration count from arguments

    benchmark_context context = {0};
    context.filename = "X:\\projects\\1billion\\measurements.txt"; // TODO: Read filename from arguments
    
    // Get file length
    struct __stat64 stat;
    _stat64(context.filename, &stat);
    context.file_length = stat.st_size;

    context.read_buffer.capacity = KiB(16);
    context.read_buffer.base = malloc(context.read_buffer.capacity);

    // Warmup run
    printf("Warmup .");
    fflush(stdout);

    u64 expected_count = RunBenchmark(0, &context);

    for (u32 benchmark = 1; benchmark < BENCHMARK_COUNT; benchmark++) {
        printf(".");
        fflush(stdout);

        u64 count = RunBenchmark(benchmark, &context);
        if (count != expected_count) {
            printf("\n Count is not the same!");
            exit(EXIT_FAILURE);
        }
    }

    free(context.read_buffer.base);

    printf(" DONE\n");

    const u64 buffer_sizes[] = {
        KiB(1), KiB(2), KiB(4), KiB(8), KiB(16), KiB(32), KiB(64), KiB(128), KiB(256), KiB(512),
        MiB(1), MiB(2), MiB(4), MiB(8), MiB(16), MiB(32), MiB(64), MiB(128), MiB(256), MiB(512)
    };

    for (u32 buffer_index = 0; buffer_index < ArrayCount(buffer_sizes); buffer_index++) {
        printf("Running[%llu] ", buffer_sizes[buffer_index]);

        context.read_buffer.capacity = buffer_sizes[buffer_index];
        context.read_buffer.base = malloc(context.read_buffer.capacity);

        dani_BeginProfiling();
    
        for (u32 i = 0; i < iteration_count; i++) {
            for (u32 benchmark = 0; benchmark < BENCHMARK_COUNT; benchmark++) {
                printf(".");
                fflush(stdout);

                u64 count = RunBenchmark(benchmark, &context);
                if (count != expected_count) {
                    printf("\n Count is not the same!");
                    exit(EXIT_FAILURE);
                }
            }
            printf(" ");
        }
    
        printf(" DONE\n");

        dani_EndProfiling();
        dani_PrintProfilingResults();

        free(context.read_buffer.base);
        printf("\n");
    }

    return EXIT_SUCCESS;
}
