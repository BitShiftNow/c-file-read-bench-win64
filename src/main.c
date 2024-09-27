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

u32 ReadIterationCountFromArguments(const s8 *argument) {
    u32 result = 0;

    if (argument) {
        u64 len = strlen(argument);
        for (u64 i = 0; i < len; i++) {
            u8 byte = argument[i];
            b32 is_digit = ((byte - '0') < 10);
            if (is_digit) {
                result = (result * 10) + (byte - '0');
                if (result > S32_MAX) {
                    printf("Iteration argument too big. Using 1 by default.\n");
                    result = 0;
                    break;
                }
            } else {
                printf("Invalid iteration count argument. Using 1 by default.\n");
                result = 0;
                break;
            }
        }
    }

    if (result == 0) {
        printf("Invalid iteration count argument. Using 1 by default.\n");
        result = 1;
    }
    return (result);
}

s32 main(s32 argc, s8 **argv) {
    if (argc < 3) {
        printf("Missing iteration count and filename argument.\n");
        exit(EXIT_FAILURE);
    }

    const u32 iteration_count = ReadIterationCountFromArguments(argv[1]);

    benchmark_context context = {0};
    context.filename = argv[2];
    
    // Get file length
    struct __stat64 stat;
    _stat64(context.filename, &stat);
    context.file_length = stat.st_size;
    if (context.file_length == 0) {
        printf("Couldn't get size of file %s\n", context.filename);
        exit(EXIT_FAILURE);
    }

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
        fflush(stdout);

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
        fflush(stdout);

        dani_EndProfiling();
        dani_PrintProfilingResults();

        free(context.read_buffer.base);
        printf("\n");
    }

    return EXIT_SUCCESS;
}
