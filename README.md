# ACCSV: ASCII Control Character Separated Values

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Version](https://img.shields.io/badge/Version-4.0.0-blue.svg)](https://github.com/yourusername/accsv/releases/tag/v4.0.0)
[![Build Status](https://img.shields.io/badge/Build-Passing-green.svg)](https://github.com/yourusername/accsv/actions)

A rediscovery of ASCII control characters for robust, collision-proof tabular data storage. Fixes CSV's delimiter hell with `US` (0x1F) for fields and `RS` (0x1E) for records. Simple, fast, no escaping needed.

## Manifesto: Looking Back to Move Forward

For 50+ years, CSV/TSV have sucked due to delimiter collisions leading to quoting/escaping nightmares. ASCII's control chars like US/RS/GS were built for this—non-printable, zero collision risk. We forgot 'em; ACCSV brings 'em back for speed and simplicity.

This ain't new invention; it's overlooked genius from 1963. Challenge: What other simple fixes have we ignored?

## File Specifications

### ACCSV Data File (`.accsv`)

Pure record stream.

| Property | Specification |
|----------|--------------|
| **Field Separator** | `US` (0x1F) |
| **Record Terminator** | `RS` (0x1E) |
| **Header Flag** | Optional `SUB` (0x1A) at byte 0 indicates first record is header. |
| **Cosmetic Newline** | Optional LF/CRLF after RS (ignored by parsers). |
| **Encoding** | Agnostic; app handles byte interpretation. |

### Manifest/Index File (`.accsv.midx`)

Hybrid: Readable metadata + binary index.

- **[Meta]** section: Key-value pairs (e.g., `Path = file.accsv`, `Algorithm = SHA256`, `Digest = hash`).
- **[IDX]** section: Binary follows.

Binary structure (little-endian):

| Part | Size | Type | Description |
|------|------|------|-------------|
| Magic | 8 | char[8] | `ACSVIDX1` |
| Version | 2 | uint16_t | e.g., 0x0100 |
| Reserved | 6 | uint8_t[6] | Zeros |
| Record Count | 8 | uint64_t | Total records |
| Offsets | N*8 | uint64_t[] | Byte offsets per record |

## Command-Line Tool: `accsv`

High-perf CLI for parsing/indexing. Subcommands like git.

Usage: `accsv <command> [options]`

Commands:
- `index <file.accsv>`: Create `.accsv.midx`.
- `count [file.accsv]`: Record count (fast with index).
- `view [file.accsv]`: Human-readable output (tabs/newlines).
- `slice <file.accsv> <start> [end]`: Extract range (-ah adds header).

Full help: `accsv --help`

## Core C Library API (`accsv.h`)

Public interface:

```c
#ifndef ACCSV_H
#define ACCSV_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

typedef struct { const char* start; size_t length; } AccsvFieldView;
typedef struct { AccsvFieldView* fields; size_t field_count; } AccsvRecordView;
typedef struct AccsvParser AccsvParser;
typedef struct AccsvIndex AccsvIndex;

AccsvParser* accsv_parser_new(FILE* stream);
const AccsvRecordView* accsv_parser_next_record(AccsvParser* parser);
void accsv_parser_free(AccsvParser* parser);
int accsv_parser_has_header(const AccsvParser* parser);

AccsvIndex* accsv_index_load(const char* midx_path);
void accsv_index_free(AccsvIndex* index);
uint64_t accsv_index_get_record_count(const AccsvIndex* index);
int accsv_parser_seek(AccsvParser* parser, const AccsvIndex* index, uint64_t record_number);

int accsv_parser_in_error_state(const AccsvParser* parser);
const char* accsv_parser_get_error(const AccsvParser* parser);

#endif
