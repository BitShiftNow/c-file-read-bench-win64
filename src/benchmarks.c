static u64 fread_version(benchmark_context *context) {
    buffer read_buffer = context->read_buffer;
    u64 total_read = 0;
    u64 result = 0;

    FILE *file = fopen(context->filename, "rb");
    if (file) {
        dani_ProfileFunction({
            while (total_read < context->file_length) {
                u64 read = fread(read_buffer.base, sizeof(u8), read_buffer.capacity, file);
                if (read < read_buffer.capacity) {
                    if (read == 0 || ferror(file)) {
                        printf("Failed to read from file %s\n", context->filename);
                        exit(EXIT_FAILURE);
                    }
                }
                total_read += read;

                // Do some simple counting
                for (u64 i = 0; i < read; i++) {
                    result += read_buffer.base[i];
                }
            }
        });
        fclose(file);
    } else {
        printf("Failed to open file %s\n", context->filename);
        exit(EXIT_FAILURE);
    }

    return (result);
}

static u64 ReadFile_version(benchmark_context *context) {
    buffer read_buffer = context->read_buffer;
    u64 total_read = 0;
    u64 result = 0;

    void *file = CreateFileA(context->filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file != INVALID_HANDLE_VALUE) {
        dani_ProfileFunction({
            while (total_read < context->file_length) {
                u32 read = 0;
                b32 read_result = ReadFile(file, read_buffer.base, (DWORD)read_buffer.capacity, (LPDWORD)&read, 0);
                if (IsFalse(read_result) || read == 0) {
                    printf("Failed to read from file %s\n", context->filename);
                    exit(EXIT_FAILURE);
                }
                total_read += read;

                // Do some simple counting
                for (u64 i = 0; i < read; i++) {
                    result += read_buffer.base[i];
                }
            }
        });
        CloseHandle(file);
    } else {
        printf("Failed to open file %s\n", context->filename);
        exit(EXIT_FAILURE);
    }

    return (result);
}

typedef enum BENCHMARKS benchmarks;
enum BENCHMARKS {
    BENCHMARK_FREAD = 0,
    BENCHMARK_READFILE = 1,

    BENCHMARK_COUNT,
};

static u64 RunBenchmark(benchmarks benchmark, benchmark_context *context) {
    switch (benchmark) {
        case BENCHMARK_FREAD: return fread_version(context);
        case BENCHMARK_READFILE: return ReadFile_version(context);
        default: return 0;
    }
}
