#ifndef ACCSV_H
#define ACCSV_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#  ifdef ACCSV_BUILD_DLL
#    define ACCSV_API __declspec(dllexport)
#  else
#    define ACCSV_API __declspec(dllimport)
#  endif
#else
#  define ACCSV_API
#endif

typedef enum {
    ACCSV_SUCCESS = 0,
    ACCSV_ERR_EOF = -1,
    ACCSV_ERR_PARTIAL_RECORD = -2,
    ACCSV_ERR_MALLOC_FAIL = -3,
    ACCSV_ERR_INVALID_MIDX = -4,
    ACCSV_ERR_SEEK_FAIL = -5,
    ACCSV_ERR_BUFFER_OVERFLOW = -6
} AccsvError;

ACCSV_API const char* accsv_get_error_desc(AccsvError code);

typedef struct { const char* start; size_t length; } AccsvFieldView;
typedef struct { AccsvFieldView* fields; size_t field_count; } AccsvRecordView;

typedef struct AccsvParser AccsvParser;
typedef struct AccsvIndex AccsvIndex;

struct AccsvParser {
    FILE* stream;
    char* buffer;
    size_t buffer_size;
    size_t data_len;
    size_t data_start;
    AccsvRecordView current_record;
    AccsvFieldView* fields;
    size_t fields_capacity;
    int has_header_flag;
    AccsvError error;
    char error_msg[256];
    uint64_t file_pos; // The absolute position in the file stream
    AccsvIndex* index;  // Optional for indexed parsers
};

struct AccsvIndex {
    uint64_t record_count;
    uint64_t* offsets;
};

typedef void (*accsv_record_callback)(const AccsvRecordView* record_view, int thread_id, void* user_context);

typedef struct {
    int num_threads;  // 0 for auto-detect
    void* user_context;
} AccsvParallelOptions;

// Tier 1: Sequential
ACCSV_API AccsvParser* accsv_parser_new(FILE* stream);
ACCSV_API int accsv_parser_next_record(AccsvParser* parser, AccsvRecordView* record);
ACCSV_API int accsv_parser_has_header(const AccsvParser* parser);
ACCSV_API void accsv_parser_free(AccsvParser* parser);

// Tier 2: Parallel
ACCSV_API int accsv_process_stream_parallel(FILE* stream, accsv_record_callback callback, AccsvParallelOptions* options);
ACCSV_API int accsv_process_mmap_parallel(const char* file_path, accsv_record_callback callback, AccsvParallelOptions* options);

// Indexing & Random Access
ACCSV_API AccsvIndex* accsv_index_load(const char* midx_path);
ACCSV_API void accsv_index_free(AccsvIndex* index);
ACCSV_API uint64_t accsv_index_get_record_count(const AccsvIndex* index);
ACCSV_API int accsv_parser_seek(AccsvParser* parser, const AccsvIndex* index, uint64_t record_number);
ACCSV_API int accsv_build_index(const char* data_path, const char* midx_path, const char* algo);
ACCSV_API int accsv_build_index_parallel(const char* data_path, const char* midx_path, const char* algo, AccsvParallelOptions* options);
ACCSV_API int accsv_index_validate(const AccsvIndex* index, const char* data_path);

// Additional Utilities
ACCSV_API int accsv_convert_csv(const char* csv_path, const char* accsv_path);
ACCSV_API int accsv_append_record(AccsvParser* parser, const AccsvRecordView* record);

#endif
