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
    u64 total_capacity;
    u64 chunk_capacity;
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

const s8 * ReadFileNameFormArguments(const s8 *argument, s8 **volume) {
    s8 *result = 0;

    u32 path_length = GetFullPathNameA(argument, 0, 0, 0);
    if (path_length > 0) {
        result = malloc(path_length);
        path_length = GetFullPathNameA(argument, path_length, result, 0);
        if (path_length == 0) {
            free(result);
            result = 0;
        } else {
            u64 count = 0;
            for (; count < path_length; count++) {
                if (result[count] == ':') {
                    count += 3;
                    break;
                }
            }
            *volume = malloc(count);
            memcpy(*volume, result, count - 1);
            (*volume)[count - 1] = '\0';
        }
    }

    return (result);
}

buffer AllocateBuffer(u64 chunk_size) {
    buffer result = {0};

    u64 total_size = chunk_size * 2;

    result.base = VirtualAlloc(0, total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (result.base != 0) {
        result.total_capacity = total_size;
        result.chunk_capacity = chunk_size;

        // Touch all pages to avoid page faults
        SYSTEM_INFO info = {0};
        GetSystemInfo(&info);

        u64 page_size = info.dwPageSize;
        for (u64 i = 0; i < result.total_capacity; i += page_size) {
            result.base[i] = S8_MAX;
        }
    }

    return (result);
}

void FreeBuffer(buffer buf) {
    if (buf.base != 0) {
        VirtualFree(buf.base, 0, MEM_RELEASE);
    }
}

s32 main(s32 argc, s8 **argv) {
    if (argc < 3) {
        printf("Missing iteration count and filename argument.\n");
        exit(EXIT_FAILURE);
    }

    const u32 iteration_count = ReadIterationCountFromArguments(argv[1]);

    s8 *volume = 0;
    benchmark_context context = {0};
    context.filename = ReadFileNameFormArguments(argv[2], &volume);
    
    // Get file length
    struct __stat64 stat;
    _stat64(context.filename, &stat);
    context.file_length = stat.st_size;
    if (context.file_length == 0) {
        printf("Couldn't get size of file %s\n", context.filename);
        exit(EXIT_FAILURE);
    }

    context.read_buffer = AllocateBuffer(MiB(2));

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

    FreeBuffer(context.read_buffer);

    printf(" DONE\n");

    const u64 buffer_sizes[] = {
        KiB(1), KiB(2), KiB(4), KiB(8), KiB(16),
        KiB(32), KiB(64), KiB(128), KiB(256), KiB(512),
        MiB(1), MiB(2), MiB(4), MiB(8), MiB(16),
        MiB(32), MiB(64), MiB(128), MiB(256), MiB(512)
    };

    for (u32 buffer_index = 0; buffer_index < ArrayCount(buffer_sizes); buffer_index++) {
        u64 buffer_length = buffer_sizes[buffer_index];
        printf("Running[%llu] ", buffer_length);
        fflush(stdout);

        context.read_buffer = AllocateBuffer(buffer_length);
        
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

        FreeBuffer(context.read_buffer);
        printf("\n");
    }

    return EXIT_SUCCESS;
}
