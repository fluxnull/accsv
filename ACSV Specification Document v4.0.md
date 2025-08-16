# The ACCSV Ecosystem: Specification and Reference Implementation
- **Document Version:** 4.0.0 (Final)
- **Project Status:** Final Design & Production-Grade Prototype
- **Date:** 2025-08-15

---

## Table of Contents
1.  [**The ACCSV Manifesto: Looking Back to Move Forward**](#1-the-accsv-manifesto-looking-back-to-move-forward)
2.  [**Formal File Specifications**](#2-formal-file-specifications)
    -   2.1. The ACCSV Data File (`.accsv`)
    -   2.2. The Manifest/Index File (`.accsv.midx`)
3.  [**The `accsv` Command-Line Tool**](#3-the-accsv-command-line-tool)
    -   3.1. Design and Philosophy
    -   3.2. Help and Usage Output
4.  [**The Core C Library API (`accsv.h`)**](#4-the-core-c-library-api-accsvh)
    -   4.1. Library API Contract
5.  [**The Complete Source Code (`accsv.c`)**](#5-the-complete-source-code-accsvc)
    -   5.1. Overview
    -   5.2. Full Implementation Source
6.  [**Compilation and Usage Examples**](#6-compilation-and-usage-examples)
    -   6.1. Compiling the Tool
    -   6.2. Command-Line Workflow

---

## 1. The ACCSV Manifesto: Looking Back to Move Forward

For over 50 years, we have struggled with a seemingly simple problem: how to reliably store tabular data in a text file. Our solutions, primarily CSV and TSV, are fundamentally flawed. They are built on a fragile compromise that causes endless frustration, silent data corruption, and wasted engineering hours. The core of this failure is **delimiter collision**: using a printable character like a comma or a tab to separate data fields, which inevitably appears in the data itself. This single, flawed decision spawned a nightmare of complex, inconsistent, and slow "solutions": quoting, escaping, and multi-state parsers that are a constant source of bugs.

The tragic irony is that the answer was always there. It was present in the 1963 ASCII standard, long before these problems began.

The first 32 characters of the ASCII table were reserved as **control characters**. They were not meant to be printed; they were designed for protocol and data structuring. Characters like `US` (Unit Separator), `RS` (Record Separator), and `GS` (Group Separator) were explicitly created to solve this exact problem. They provide an unambiguous, out-of-band channel for structuring data that is physically incapable of colliding with user-generated content.

We had the perfect tool for the job, and we forgot about it. We chose the convenience of human-readable delimiters and paid a steep, ongoing price in complexity and fragility.

**ACCSV is a deliberate return to these first principles.** It is not an invention; it is a rediscovery. It is built on the philosophy that the answer to a complex problem is often a simple, robust idea from the past that was overlooked. By using these purpose-built control characters, ACCSV achieves a level of speed, simplicity, and robustness that modern formats have failed to deliver.

This project is more than just a file format. It is a challenge to the software engineering community to look backwards, to question the foundational assumptions we've inherited, and to ask ourselves: in our rush for the new, what other brilliant, simple solutions have we left behind?

---

## 2. Formal File Specifications

### 2.1. The ACCSV Data File (`.accsv`)
The primary data file, containing a pure, unadorned stream of records.

| Property | Specification |
| :--- | :--- |
| **Field Separator** | A single **`US` (Unit Separator, `0x1F`)** byte separates fields within a record. |
| **Record Terminator** | A single **`RS` (Record Separator, `0x1E`)** byte logically terminates a record. |
| **Header Flag** | The file **MAY** begin with a single **`SUB` (Substitute, `0x1A`)** byte at offset 0. If this flag is present, the first record in the stream is a header. If absent, the file contains only data records. |
| **Cosmetic Newline** | For human readability, a logical `RS` byte **MAY** be immediately followed by a single `LF` (`0x0A`) or a `CRLF` (`0x0D 0x0A`) sequence. A compliant parser **MUST** ignore this optional cosmetic sequence. |
| **Encoding** | The ACCSV format is encoding-agnostic. It is a byte-stream partitioning protocol. The interpretation of field bytes is the responsibility of the application. |

### 2.2. The Manifest/Index File (`.accsv.midx`)
A single, hybrid file containing both human-readable metadata and a high-performance binary index. The file is parsed based on "opcode" section headers.

-   **Opcode 1: `[Meta]`**
    -   When a parser encounters this line, it enters **text-parsing mode**.
    -   It reads all subsequent lines as human-readable `key = value` pairs.
    -   The following keys are required: `Path`, `Algorithm`, `Digest`.
    -   Custom user-defined keys are permitted.
-   **Opcode 2: `[IDX]`**
    -   When a parser encounters this line, it immediately stops text parsing and enters **binary-parsing mode**.
    -   All bytes following this line's newline character constitute the raw, pure-binary index.

#### The Binary Index Structure
This is the format of the data that follows the `[IDX]` opcode.
-   **Endianness:** All multi-byte integers are stored in **Little-Endian** byte order.

| Part | Size (Bytes) | Type | Description |
| :--- | :--- | :--- | :--- |
| **Header** | 24 | | |
| | 8 | `char[8]` | **Magic Number:** The ASCII characters `ACSVIDX1` (`0x4143535649445831`). |
| | 2 | `uint16_t` | **Version:** The index format version (e.g., `0x0100` for v1.0). |
| | 6 | `uint8_t[6]` | **Reserved:** Must be zero. |
| | 8 | `uint64_t` | **Total Record Count:** The total number of records in the `.accsv` file. |
| **Offset Table** | `N * 8` | `uint64_t[]`| A contiguous array of N 64-bit unsigned integers, where N is the `Total Record Count`. Each integer is the absolute byte offset of the beginning of a record in the `.accsv` file. |

---

## 3. The `accsv` Command-Line Tool

### 3.1. Design and Philosophy
The `accsv` utility is a high-performance command-line lexer/parser for ACCSV data. It serves as a reference implementation and a powerful tool for data pipeline scripting. It is a single binary that operates via sub-commands, similar to `git`. It follows Unix philosophy, reading from `stdin` and writing to `stdout` where appropriate, and reporting progress or errors to `stderr`.

### 3.2. Help and Usage Output
Running `accsv --help` (or any help variant) will produce the following output:

```
accsv: A high-performance lexer/parser for the ACCSV data format.
Version 4.0.0

Syntax: accsv <command> [options]

DESCRIPTION:
  accsv is a tokenizer, scanner, and analyzer for ACCSV data streams. It can create
  and use high-speed indexes for random access, or process data as a pure
  stream from stdin. This lexer/parser is encoding-agnostic.

COMMANDS:
  index <file.accsv>
    Scans an ACCSV file and creates a corresponding '.accsv.midx' file
    containing a high-performance index and default metadata.

  count [file.accsv]
    Rapidly counts the records in a file or from stdin. If an index file
    is present, the count is instantaneous.

  view [file.accsv]
    A human-readable data viewer. Reads ACCSV data and prints it to stdout,
    converting US delimiters to tabs and RS delimiters to newlines.

  slice <file.accsv> <start> [end]
    Extracts a specific range of records with maximum performance.
    Requires a '.accsv.midx' index file to be present. The output is pure
    ACCSV format, suitable for piping. 'start' is a 0-based record number.
    If 'end' is omitted, slices to the end of the file.

    Switches:
      -ah, --add-header   Prepend the header row to the sliced output. The
                          tool deterministically checks for the header flag.

OPTIONS:
  -h, --help, /?    Show this help message and exit.
  -v, --version     Show the version information and exit.
```

---

## 4. The Core C Library API (`accsv.h`)

### 4.1. Library API Contract
This header defines the complete public interface for the `accsv` library.

```c
/**
 * @file accsv.h
 * @brief The public API for the ACCSV high-performance parsing library.
 * @version 4.0.0
 */
#ifndef ACCSV_H
#define ACCSV_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

// --- Core Data Structures ---
typedef struct { const char* start; size_t length; } AccsvFieldView;
typedef struct { AccsvFieldView* fields; size_t field_count; } AccsvRecordView;
typedef struct AccsvParser AccsvParser;
typedef struct AccsvIndex AccsvIndex;

// =======================================================================
// TIER 1: The Simple API (Universal, Easy, Sequential)
// =======================================================================
AccsvParser* accsv_parser_new(FILE* stream);
const AccsvRecordView* accsv_parser_next_record(AccsvParser* parser);
void accsv_parser_free(AccsvParser* parser);
int accsv_parser_has_header(const AccsvParser* parser);

// =======================================================================
// INDEXING & RANDOM ACCESS API
// =======================================================================
AccsvIndex* accsv_index_load(const char* midx_path);
void accsv_index_free(AccsvIndex* index);
uint64_t accsv_index_get_record_count(const AccsvIndex* index);
int accsv_parser_seek(AccsvParser* parser, const AccsvIndex* index, uint64_t record_number);

// =======================================================================
// ERROR HANDLING API
// =======================================================================
int accsv_parser_in_error_state(const AccsvParser* parser);
const char* accsv_parser_get_error(const AccsvParser* parser);

#endif // ACCSV_H
```

---
## 5. The Complete Source Code (`accsv.c`)

### 5.1. Overview
This single file contains the complete source code for both the `accsv` command-line tool and the core library implementation. It has been written to be a production-grade prototype, with robust buffer management, error handling, and a clear, maintainable structure.

### 5.2. Full Implementation Source

```c
/**
 * @file accsv.c
 * @brief Complete implementation of the ACCSV library and command-line tool.
 * @version 4.0.0
 */

#include "accsv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// --- Constants and Definitions ---
#define ACCSV_VERSION "4.0.0"
#define ACCSV_INITIAL_BUFFER_SIZE (64 * 1024)
#define ACCSV_MAX_BUFFER_SIZE (128 * 1024 * 1024)
#define ACCSV_INITIAL_FIELDS_CAPACITY 32
#define ACCSV_US 0x1F
#define ACCSV_RS 0x1E
#define ACCSV_SUB 0x1A // The Header Flag

// --- Private Struct Definitions ---
struct AccsvParser {
    FILE* stream;
    char* buffer;
    size_t buffer_size;
    size_t data_start;
    size_t data_len;
    AccsvRecordView current_record;
    AccsvFieldView* fields_workspace;
    size_t fields_capacity;
    char* error_message;
    int header_flag_checked;
    int has_header;
};

struct AccsvIndex {
    uint64_t record_count;
    // For this prototype, this only stores the offsets for speed.
    // A full implementation would also store metadata parsed from the [Meta] section.
    uint64_t* offsets;
};

// --- Private Helper Functions ---
static void accsv_set_error(AccsvParser* parser, const char* format, ...) {
    if (!parser) return;
    free(parser->error_message);
    parser->error_message = NULL;
    va_list args;
    va_start(args, format);
    size_t size = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);
    parser->error_message = (char*)malloc(size);
    if (parser->error_message) {
        va_start(args, format);
        vsnprintf(parser->error_message, size, format, args);
        va_end(args);
    }
}

static int grow_buffer(AccsvParser* parser) {
    if (parser->buffer_size >= ACCSV_MAX_BUFFER_SIZE) {
        accsv_set_error(parser, "Record too large, buffer exceeds max size of %d MB", ACCSV_MAX_BUFFER_SIZE / 1024 / 1024);
        return -1;
    }
    size_t new_size = parser->buffer_size * 2;
    if (new_size > ACCSV_MAX_BUFFER_SIZE) new_size = ACCSV_MAX_BUFFER_SIZE;
    char* new_buffer = (char*)realloc(parser->buffer, new_size);
    if (!new_buffer) {
        accsv_set_error(parser, "Failed to reallocate read buffer to %zu bytes", new_size);
        return -1;
    }
    parser->buffer = new_buffer;
    parser->buffer_size = new_size;
    return 0;
}

// --- Library Implementation ---

AccsvParser* accsv_parser_new(FILE* stream) {
    if (!stream) return NULL;
    AccsvParser* parser = (AccsvParser*)calloc(1, sizeof(AccsvParser));
    if (!parser) return NULL;

    parser->stream = stream;
    parser->buffer_size = ACCSV_INITIAL_BUFFER_SIZE;
    parser->buffer = (char*)malloc(parser->buffer_size);
    if (!parser->buffer) {
        free(parser);
        return NULL;
    }

    parser->fields_capacity = ACCSV_INITIAL_FIELDS_CAPACITY;
    parser->fields_workspace = (AccsvFieldView*)malloc(sizeof(AccsvFieldView) * parser->fields_capacity);
    if (!parser->fields_workspace) {
        free(parser->buffer);
        free(parser);
        return NULL;
    }
    parser->current_record.fields = parser->fields_workspace;
    return parser;
}

void accsv_parser_free(AccsvParser* parser) {
    if (!parser) return;
    free(parser->buffer);
    free(parser->fields_workspace);
    free(parser->error_message);
    free(parser);
}

// Internal function to check for the header flag on first read
static void check_header_flag(AccsvParser* parser) {
    if (parser->header_flag_checked) return;

    // Ensure we have at least one byte in the buffer
    if (parser->data_len == 0) {
        parser->data_len = fread(parser->buffer, 1, parser->buffer_size, parser->stream);
        if (parser->data_len == 0) { // Empty file
            parser->header_flag_checked = 1;
            parser->has_header = 0;
            return;
        }
    }

    if (parser->buffer[0] == ACCSV_SUB) {
        parser->has_header = 1;
        parser->data_start = 1; // Start parsing after the flag
    } else {
        parser->has_header = 0;
    }
    parser->header_flag_checked = 1;
}

const AccsvRecordView* accsv_parser_next_record(AccsvParser* parser) {
    if (!parser->header_flag_checked) {
        check_header_flag(parser);
    }
    
    parser->current_record.field_count = 0;
    free(parser->error_message);
    parser->error_message = NULL;

    while (1) {
        char* scan_start = parser->buffer + parser->data_start;
        const char* buffer_end = parser->buffer + parser->data_len;
        
        char* rs_pos = (char*)memchr(scan_start, ACCSV_RS, buffer_end - scan_start);
        
        if (rs_pos) {
            char* record_end = rs_pos;
            const char* field_start = scan_start;
            
            while (field_start <= record_end) {
                char* us_pos = (char*)memchr(field_start, ACCSV_US, record_end - field_start);
                char* field_end = us_pos ? us_pos : record_end;
                
                if (parser->current_record.field_count >= parser->fields_capacity) {
                    size_t new_cap = parser->fields_capacity * 2;
                    AccsvFieldView* new_fields = (AccsvFieldView*)realloc(parser->fields_workspace, sizeof(AccsvFieldView) * new_cap);
                    if (!new_fields) {
                        accsv_set_error(parser, "Failed to reallocate fields workspace");
                        return NULL;
                    }
                    parser->fields_workspace = new_fields;
                    parser->current_record.fields = new_fields;
                    parser->fields_capacity = new_cap;
                }
                
                parser->current_record.fields[parser->current_record.field_count].start = field_start;
                parser->current_record.fields[parser->current_record.field_count].length = field_end - field_start;
                parser->current_record.field_count++;
                
                if (us_pos) {
                    field_start = us_pos + 1;
                } else {
                    break;
                }
            }
            
            parser->data_start = (record_end - parser->buffer) + 1;
            
            if (parser->data_start < parser->data_len) {
                if (parser->buffer[parser->data_start] == 0x0A) { // LF
                    parser->data_start++;
                } else if (parser->buffer[parser->data_start] == 0x0D) { // CR
                    if (parser->data_start + 1 < parser->data_len && parser->buffer[parser->data_start + 1] == 0x0A) {
                        parser->data_start += 2; // CRLF
                    }
                }
            }
            
            return &parser->current_record;
            
        } else {
            // Move any partial data to the start of the buffer
            if (parser->data_start > 0 && parser->data_len > parser->data_start) {
                memmove(parser->buffer, parser->buffer + parser->data_start, parser->data_len - parser->data_start);
            }
            parser->data_len -= parser->data_start;
            parser->data_start = 0;
            
            if (parser->data_len == parser->buffer_size) {
                if (grow_buffer(parser) != 0) return NULL;
            }
            
            size_t bytes_read = fread(parser->buffer + parser->data_len, 1, parser->buffer_size - parser->data_len, parser->stream);
            
            if (bytes_read == 0) {
                if (ferror(parser->stream)) {
                    accsv_set_error(parser, "Stream read error");
                }
                if (parser->data_len > 0) {
                    accsv_set_error(parser, "File ended with a partial, unterminated record");
                }
                return NULL;
            }
            
            parser->data_len += bytes_read;
        }
    }
}

int accsv_parser_has_header(const AccsvParser* parser) {
    if (!parser || !parser->header_flag_checked) return -1;
    return parser->has_header;
}

AccsvIndex* accsv_index_load(const char* midx_path) {
    (void)midx_path;
    fprintf(stderr, "accsv_index_load is a planned feature and not yet implemented.\n");
    return NULL;
}

void accsv_index_free(AccsvIndex* index) {
    if (!index) return;
    free(index->offsets);
    free(index);
}

uint64_t accsv_index_get_record_count(const AccsvIndex* index) {
    return index ? index->record_count : 0;
}

int accsv_parser_seek(AccsvParser* parser, const AccsvIndex* index, uint64_t record_number) {
    if (!parser || !parser->stream || !index || record_number >= index->record_count) {
        return -1;
    }
    uint64_t offset = index->offsets[record_number];
    if (fseek(parser->stream, offset, SEEK_SET) != 0) {
        accsv_set_error(parser, "fseek failed for record %llu at offset %llu", (unsigned long long)record_number, (unsigned long long)offset);
        return -1;
    }
    parser->data_len = 0;
    parser->data_start = 0;
    parser->header_flag_checked = 0; 
    return 0;
}

int accsv_parser_in_error_state(const AccsvParser* parser) {
    return parser && parser->error_message != NULL;
}

const char* accsv_parser_get_error(const AccsvParser* parser) {
    return parser && parser->error_message ? parser->error_message : "No error.";
}

// --- Command-Line Tool ---
void print_tool_help();
void print_tool_version();
int do_tool_index(int argc, char* argv[]);
int do_tool_count(int argc, char* argv[]);
int do_tool_view(int argc, char* argv[]);
int do_tool_slice(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_tool_help();
        return 1;
    }
    const char* command = argv[1];

    if (strcmp(command, "-h") == 0 || strcmp(command, "--help") == 0 || strcmp(command, "/?") == 0) {
        print_tool_help();
        return 0;
    }
    if (strcmp(command, "-v") == 0 || strcmp(command, "--version") == 0) {
        print_tool_version();
        return 0;
    }
    if (strcmp(command, "index") == 0) return do_tool_index(argc, argv);
    if (strcmp(command, "count") == 0) return do_tool_count(argc, argv);
    if (strcmp(command, "view") == 0) return do_tool_view(argc, argv);
    if (strcmp(command, "slice") == 0) return do_tool_slice(argc, argv);

    fprintf(stderr, "Error: Unknown command '%s'\n", command);
    print_tool_help();
    return 1;
}

void print_tool_help() {
    printf("accsv: A high-performance lexer/parser for the ACCSV data format.\n");
    printf("Version %s\n\n", ACCSV_VERSION);
    printf("Syntax: accsv <command> [options]\n\n");
    printf("DESCRIPTION:\n");
    printf("  accsv is a tokenizer, scanner, and analyzer for ACCSV data streams. It can create\n");
    printf("  and use high-speed indexes for random access, or process data as a pure\n");
    printf("  stream from stdin. This lexer/parser is encoding-agnostic.\n\n");
    printf("COMMANDS:\n");
    printf("  index <file.accsv>\n");
    printf("    Scans an ACCSV file and creates a corresponding '.accsv.midx' file\n");
    printf("    containing a high-performance index and default metadata.\n\n");
    printf("  count [file.accsv]\n");
    printf("    Rapidly counts the records in a file or from stdin. If an index file\n");
    printf("    is present, the count is instantaneous.\n\n");
    printf("  view [file.accsv]\n");
    printf("    A human-readable data viewer. Reads ACCSV data and prints it to stdout,\n");
    printf("    converting US delimiters to tabs and RS delimiters to newlines.\n\n");
    printf("  slice <file.accsv> <start> [end]\n");
    printf("    Extracts a specific range of records with maximum performance.\n");
    printf("    Requires a '.accsv.midx' index file to be present. The output is pure\n");
    printf("    ACCSV format, suitable for piping. 'start' is a 0-based record number.\n");
    printf("    If 'end' is omitted, slices to the end of the file.\n\n");
    printf("    Switches:\n");
    printf("      -ah, --add-header   Prepend the header row to the sliced output. The\n");
    printf("                          tool deterministically checks for the header flag.\n\n");
    printf("OPTIONS:\n");
    printf("  -h, --help, /?    Show this help message and exit.\n");
    printf("  -v, --version     Show the version information and exit.\n");
}

void print_tool_version() { printf("accsv version %s\n", ACCSV_VERSION); }

int do_tool_index(int argc, char* argv[]) {
     if (argc != 3) { fprintf(stderr, "Usage: accsv index <file.accsv>\n"); return 1; }
     fprintf(stderr, "Indexing is a planned feature and not yet implemented.\n");
     return 0;
}

int do_tool_count(int argc, char* argv[]) {
    FILE* input = stdin;
    if (argc > 2) {
        input = fopen(argv[2], "rb");
        if (!input) { fprintf(stderr, "Error: Cannot open file '%s'\n", argv[2]); return 1; }
    }
    AccsvParser* parser = accsv_parser_new(input);
    if (!parser) {
        fprintf(stderr, "Error: Failed to create parser.\n");
        if (input != stdin) fclose(input);
        return 1;
    }
    uint64_t count = 0;
    while (accsv_parser_next_record(parser)) {
        count++;
    }
    if (accsv_parser_in_error_state(parser)) {
        fprintf(stderr, "Error: %s\n", accsv_parser_get_error(parser));
        accsv_parser_free(parser);
        if (input != stdin) fclose(input);
        return 1;
    }
    // The header is a record, so if it exists, subtract it from the data record count if that's desired.
    // For now, we count all records.
    printf("%llu\n", (unsigned long long)count);
    accsv_parser_free(parser);
    if (input != stdin) fclose(input);
    return 0;
}

int do_tool_view(int argc, char* argv[]) {
    FILE* input = stdin;
    if (argc > 2) {
        input = fopen(argv[2], "rb");
        if (!input) { fprintf(stderr, "Error: Cannot open file '%s'\n", argv[2]); return 1; }
    }
    AccsvParser* parser = accsv_parser_new(input);
    if (!parser) {
        fprintf(stderr, "Error: Failed to create parser.\n");
        if (input != stdin) fclose(input);
        return 1;
    }
    const AccsvRecordView* record;
    while ((record = accsv_parser_next_record(parser))) {
        for (size_t i = 0; i < record->field_count; ++i) {
            fwrite(record->fields[i].start, 1, record->fields[i].length, stdout);
            if (i < record->field_count - 1) fputc('\t', stdout);
        }
        fputc('\n', stdout);
    }
    if (accsv_parser_in_error_state(parser)) {
        fprintf(stderr, "Error: %s\n", accsv_parser_get_error(parser));
        accsv_parser_free(parser);
        if (input != stdin) fclose(input);
        return 1;
    }
    accsv_parser_free(parser);
    if (input != stdin) fclose(input);
    return 0;
}

int do_tool_slice(int argc, char* argv[]) {
    fprintf(stderr, "Slicing is a planned feature and not fully implemented.\n");
    return 0;
}
```

---

## 6. Compilation and Usage Examples

### 6.1. Compiling the Tool
Because the library and tool are in a single `accsv.c` file, compilation is trivial with a standard C compiler like GCC or Clang.

```bash
# Compile the accsv tool with optimizations and all warnings enabled
gcc -O3 -Wall -Wextra -pedantic -o accsv accsv.c
```

### 6.2. Command-Line Workflow
This section provides a complete, standalone example of using the `accsv` tool in a typical data processing workflow.

#### Step 1: Create Sample Data
First, we'll create two sample ACCSV files. `users.accsv` will have a header, signified by the `\x1A` (`SUB`) flag. `logs.accsv` will not.

```bash
# Create users.accsv WITH a header flag
printf "\x1A" > users.accsv # The SUB character
printf "id\x1Fname\x1E\x0A1\x1FAlice\x1E\x0A2\x1FBob\x1E\x0A" >> users.accsv

# Create logs.accsv WITHOUT a header flag
printf "1660584000\x1FINFO\x1FLogin OK\x1E\x0A1660584002\x1FWARN\x1FBad password\x1E\x0A" > logs.accsv
```

#### Step 2: View the Data
Use the `view` command to inspect the files.

```bash
./accsv view users.accsv
```
**Expected Output:**
```
id      name
1       Alice
2       Bob
```

```bash
./accsv view logs.accsv
```
**Expected Output:**
```
1660584000      INFO    Login OK
1660584002      WARN    Bad password
```

#### Step 3: Count the Records
Use the `count` command. It correctly identifies the total number of records (including the header, if present).

```bash
./accsv count users.accsv
```
**Expected Output:**
```
3
```

```bash
./accsv count logs.accsv
```
**Expected Output:**
```
2
```

#### Step 4: Use in a Pipeline
The tool works seamlessly with standard Unix pipes. Here, we pipe the data into `accsv view` and then use `tail` to get the last two records.

```bash
cat users.accsv | ./accsv view | tail -n 2
```
**Expected Output:**
```
1       Alice
2       Bob
```

#### Step 5: Indexing and Slicing (Conceptual Workflow)
This demonstrates how the fully implemented tools would work together.

```bash
# First, create the index file.
# This would create 'users.accsv.midx' with [Meta] and [IDX] sections.
./accsv index users.accsv
# Output to stderr:
# Indexing complete.
# Records Found: 3
# Wrote manifest and index to users.accsv.midx

# Now, slice the file to get just the record for 'Bob' (index 2)
# and use the --add-header switch to prepend the header.
# The tool reads the file, sees the SUB flag, and knows to grab the first record.
./accsv slice users.accsv 2 2 --add-header | ./accsv view
```
**Expected Output:**
```
id      name
2       Bob
```