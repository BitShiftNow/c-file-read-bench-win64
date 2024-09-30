static u64 fread_version(benchmark_context *context) {
    buffer read_buffer = context->read_buffer;
    u64 total_read = 0;
    u64 result = 0;

    FILE *file = fopen(context->filename, "rb");
    if (file) {
        dani_ProfileFunctionBandwidth(context->file_length, {
            while (total_read < context->file_length) {
                u64 read = fread(read_buffer.base, sizeof(u8), read_buffer.chunk_capacity, file);
                if (read < read_buffer.chunk_capacity) {
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
        dani_ProfileFunctionBandwidth(context->file_length, {
            while (total_read < context->file_length) {
                u32 read = 0;
                b32 read_result = ReadFile(file, read_buffer.base, (DWORD)read_buffer.chunk_capacity, (LPDWORD)&read, 0);
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

static u64 ReadFileSequential_version(benchmark_context *context) {
    buffer read_buffer = context->read_buffer;
    u64 total_read = 0;
    u64 result = 0;

    void *file = CreateFileA(context->filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
    if (file != INVALID_HANDLE_VALUE) {
        dani_ProfileFunctionBandwidth(context->file_length, {
            while (total_read < context->file_length) {
                u32 read = 0;
                b32 read_result = ReadFile(file, read_buffer.base, (DWORD)read_buffer.chunk_capacity, (LPDWORD)&read, 0);
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

static b32 ReadFromFileOverlapped(void *file, OVERLAPPED *overlapped, u8 *base, u64 capacity) {
    b32 result = B32_SUCCESS;

    b32 read_result = ReadFile(file, base, (DWORD)capacity, 0, overlapped);

    if (IsFailure(read_result)) {
        b32 is_io_pending = (GetLastError() == ERROR_IO_PENDING);
        if (IsFalse(is_io_pending)) {
            result = B32_FAILURE;
        }
    }

    return (result);
}

static b32 WaitForOverlappedReadToFinish(void *file, OVERLAPPED *overlapped, u32 *read) {
    b32 result = B32_SUCCESS;

    *read = 0;
    b32 read_result = GetOverlappedResult(file, overlapped, (LPDWORD)read, B32_TRUE);
    
    if (IsFailure(read_result)) {
        b32 is_end_of_file = (GetLastError() ==  ERROR_HANDLE_EOF);
        if (IsFalse(is_end_of_file)) {
            result = B32_FAILURE;
        }
    }

    return (result);
}

static u64 ReadFileOverlapped_version(benchmark_context *context) {
    buffer read_buffer = context->read_buffer;

    u64 total_read = 0;
    u64 current_offset = 0;
    u64 next_offset = 0;
    u64 result = 0;

    void *file = CreateFileA(context->filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
    if (file != INVALID_HANDLE_VALUE) {
        dani_ProfileFunctionBandwidth(context->file_length, {
            // Read first chunk from file in a blocking fashion
            u32 read = 0;
            OVERLAPPED overlapped = {0};
            b32 read_result = ReadFile(file, &read_buffer.base[current_offset], (DWORD)read_buffer.chunk_capacity, 0, &overlapped);
            if (IsFailure(read_result)) {
                b32 is_io_pending = (GetLastError() == ERROR_IO_PENDING);
                if (is_io_pending) {
                    read_result = GetOverlappedResult(file, &overlapped, (LPDWORD)&read, B32_TRUE);
                    if (IsFailure(read_result)) {
                        printf("Failed to read from file %s\n", context->filename);
                        exit(EXIT_FAILURE);
                    }
                } else {
                    printf("Failed to read from file %s\n", context->filename);
                    exit(EXIT_FAILURE);
                }
            }
            total_read += read;
            next_offset = (current_offset + read_buffer.chunk_capacity) % read_buffer.total_capacity;

            // Read the next chunk from file in the background
            overlapped = (OVERLAPPED){0};
            overlapped.Offset     = (total_read&0x00000000FFFFFFFFull);
            overlapped.OffsetHigh = (total_read&0xFFFFFFFF00000000ull) >> 32;

            read_result = ReadFromFileOverlapped(file, &overlapped, &read_buffer.base[next_offset], read_buffer.chunk_capacity);
            if (IsFailure(read_result)) {
                printf("Failed to queue read from file %s\n", context->filename);
                exit(EXIT_FAILURE);
            }

            // Do some simple counting on the first chunk
            for (u64 i = current_offset; i < (current_offset + read); i++) {
                result += read_buffer.base[i];
            }

            while (total_read < context->file_length) {
                // Wait for the next chunk to be available
                read_result = WaitForOverlappedReadToFinish(file, &overlapped, &read);
                if (IsFailure(read_result)) {
                    printf("Failed to finish read from file %s\n", context->filename);
                    exit(EXIT_FAILURE);
                }
                total_read += read;
                current_offset = next_offset;
                next_offset = (next_offset + read_buffer.chunk_capacity) % read_buffer.total_capacity;

                if (total_read < context->file_length) {
                    // Read the next chunk from file in the background
                    overlapped = (OVERLAPPED){0};
                    overlapped.Offset     = (total_read&0x00000000FFFFFFFFull);
                    overlapped.OffsetHigh = (total_read&0xFFFFFFFF00000000ull) >> 32;

                    read_result = ReadFromFileOverlapped(file, &overlapped, &read_buffer.base[next_offset], read_buffer.chunk_capacity);
                    if (IsFailure(read_result)) {
                        printf("Failed to queue read from file %s\n", context->filename);
                        exit(EXIT_FAILURE);
                    }
                }

                // Do some simple counting on the first chunk
                for (u64 i = current_offset; i < (current_offset + read); i++) {
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

static u64 FileMapping_version(benchmark_context *context) {
    u64 result = 0;

    void *file = CreateFileA(context->filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file != INVALID_HANDLE_VALUE) {
        void *mapping = CreateFileMapping(file, 0, PAGE_READONLY, 0, 0, 0);
        if (mapping == INVALID_HANDLE_VALUE || mapping == 0) {
            printf("Failed to create mapping for file %s\n", context->filename);
            exit(EXIT_FAILURE);
        } else {
            u8 *base = (u8 *)MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, context->file_length);
            if (base == 0) {
                printf("Failed to create view for file %s\n", context->filename);
                exit(EXIT_FAILURE);
            } else {
               dani_ProfileFunctionBandwidth(context->file_length, {
                    // Do some counting
                    for (u64 i = 0; i < context->file_length; i++) {
                        result += base[i];
                    }
                });

                UnmapViewOfFile(base);
            }
            CloseHandle(mapping);
        }
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
    BENCHMARK_READFILE,
    BENCHMARK_READFILE_SEQUENTIAL,
    BENCHMARK_READFILE_OVERLAPPED,
    BENCHMARK_FILEMAPPING,

    // Not a value but the total count of values
    BENCHMARK_COUNT,
};

static u64 RunBenchmark(benchmarks benchmark, benchmark_context *context) {
    switch (benchmark) {
        case BENCHMARK_FREAD: return fread_version(context);
        case BENCHMARK_READFILE: return ReadFile_version(context);
        case BENCHMARK_READFILE_SEQUENTIAL: return ReadFileSequential_version(context);
        case BENCHMARK_READFILE_OVERLAPPED: return ReadFileOverlapped_version(context);
        case BENCHMARK_FILEMAPPING: return FileMapping_version(context);
        default: return 0;
    }
}
