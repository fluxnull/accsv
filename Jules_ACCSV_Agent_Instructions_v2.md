2025-08-18 00:00:00

# Jules Agent Instruction Pack — Complete ACCSV Build, Test, and Packaging
**Document:** ACCSV Specification Document v5.0.md  
**Goal:** Finish any missing implementation, then produce tested binaries and libraries for Windows (x64 & x86), WebAssembly, and native TypeScript/JavaScript parsers—**using the ACCSV spec exactly**.  

---

## 0) Mission Instructions (TL;DR)
- Parse and ingest **ACCSV Specification Document v5.0.md**, extract all normative requirements.
- Complete **accsv.c** where placeholders exist (parallel processing, index build/load/validate, CSV conversion, append, CLI main).
- Ensure **accsv.h** signatures match final implementation; add export macros for Windows DLLs.
- Implement/finish CLI commands: `index`, `count`, `view`, `slice`, `convert-csv` (+ options).
- Build **Windows x64/x86**: `accsv.exe` and `accsv.dll` (+ .lib import libs).
- Build **WASM** target (Node + browser) with a thin JS loader and TypeScript typings.
- Implement **native TS** and **native JS** parsers (pure language implementations).
- Create exhaustive tests: unit, golden, property, fuzz, performance. Verify equivalence across C/WASM/TS parsers.
- Ship artifacts + CI + release notes with checksums; **no “ACSV” spelling** anywhere.

---

## 1) Inputs & Ground Truth
- **Primary spec**: `ACCSV Specification Document v5.0.md` (this is the single source of truth).
- **Scope-defining bytes and rules** (from spec, enforce precisely):
  - Field sep: **US `0x1F`**
  - Record terminator: **RS `0x1E`**
  - Header flag at offset 0 (optional): **SUB `0x1A`**
  - Cosmetic newline after RS: `LF` (`0x0A`) or `CRLF` (`0x0D 0x0A`) **ignored by parser**
  - Encoding-agnostic byte protocol; fields **must not** contain raw `US/RS/SUB`
  - Max record size: **128 MiB**, error on exceed
  - Empty file OK; zero-field records OK; malformed SUB → partial record error
- **Index file (`.accsv.midx`) requirements**:
  - Hybrid file: `[Meta]` text section (key = value) then `[IDX]` binary payload
  - Binary header: magic `ACSVIDX1`, LE integers, version `0x0100` (as example), reserved zeros (6 bytes), total record count (u64)
  - Offsets array: N * u64 absolute byte offsets for each record start in the `.accsv` file

---

## 2) Deliverables
### 2.1 Windows (x64 + x86)
- `dist/windows/x64/accsv.exe`
- `dist/windows/x64/accsv.dll` + `accsv.lib` (import library), PDBs for Release/RelWithDebInfo
- `dist/windows/x86/accsv.exe`
- `dist/windows/x86/accsv.dll` + `accsv.lib`, PDBs
- All binaries must report `accsv --version` → **5.0.0** (or the spec version) and show **ACCSV** prefix everywhere.

### 2.2 WebAssembly
- `dist/wasm/accsv.wasm`
- `dist/wasm/accsv.js` (Emscripten or equivalent loader, MODULARIZE build)  
- `dist/wasm/accsv.d.ts` (TypeScript declarations)

### 2.3 Native Parsers in TS/JS
- **TS (pure)**: `packages/accsv-ts/dist/index.js`, `index.d.ts`
- **JS (pure)**: `packages/accsv-js/dist/index.js` (ESM + CJS)
- Both provide streaming + whole-buffer APIs, strict conformance to spec, and parity with C/WASM.

### 2.4 Tests & Reports
- Unit, property, fuzz, golden, performance results
- Cross-impl equivalence matrices (C vs WASM vs TS/JS)
- Coverage reports (TS/JS), memory-safety logs (C), fuzz crashers (if any) minimized and documented

### 2.5 CI & Release
- CI pipelines (Windows x64/x86, Emscripten, Node/TS) producing signed artifacts
- Checksums (SHA-256) for all deliverables
- A CHANGELOG entry and short “How to Use” README sections per target

---

## 3) Implementation Tasks
### 3.1 Fill in Missing C Implementation (accsv.c)
Implement the following to match **accsv.h** and the spec:
- `accsv_process_stream_parallel(...)`
- `accsv_process_mmap_parallel(...)`
- `accsv_index_load(...)`
- `accsv_parser_seek(...)`
- `accsv_build_index(...)`
- `accsv_build_index_parallel(...)`
- `accsv_index_validate(...)`
- `accsv_convert_csv(...)`
- `accsv_append_record(...)`
- `main(int argc, char** argv)` with subcommands & options **exactly** as documented

#### Notes & Constraints
- Use `memchr` scanning; zero-copy field views; cap record buffer at **128 MiB**
- Respect cosmetic newline after `RS`
- Strict SUB detection at offset 0 → auto header handling in `count`, `view`, `slice`
- Index builder:
  - Compute `Total Record Count`, write offsets array (u64, LE)
  - `[Meta]` must minimally contain: `Path`, `Algorithm`, `Digest`
  - Provide `--algo` choice (e.g., `MD5`, `SHA256`); compute digest over `.accsv` file
  - Note: The source code for xxhash and blake3 is located locally in Jules' VM in the /app directory. To access: cd /app; git clone https://github.com/BLAKE3-team/BLAKE3 blake3; git clone https://github.com/Cyan4973/xxHash xxhash
- CSV converter:
  - Implement CSV state machine with RFC-4180-compatible quoting, commas, CRLF/ LF handling
  - Detect header (first line) and **prepend SUB** to output if header detected
- CLI behavior must return proper error codes, stderr messages, and support stdin/stdout piping

### 3.2 Header Export Macros for Windows DLL
- In `accsv.h` add:
  ```c
  #if defined(_WIN32) || defined(__CYGWIN__)
  #  ifdef ACCSV_BUILD_DLL
  #    define ACCSV_API __declspec(dllexport)
  #  else
  #    define ACCSV_API __declspec(dllimport)
  #  endif
  #else
  #  define ACCSV_API
  #endif
  ```
- Apply `ACCSV_API` to public symbols. Keep C ABI stable; avoid C++ name mangling.

### 3.3 CLI UX Consistency
- `accsv --help` prints command list and options needed in the spec
- `accsv --version` prints version and “ACCSV” branding only
- `view` renders US as tabs and RS as newlines for readability

---

## 4) Build System & Commands
### 4.1 CMake (Preferred)
Create `CMakeLists.txt` that builds:
- Static lib `accsv_static`
- Shared lib `accsv` (→ `accsv.dll` on Windows), define `ACCSV_BUILD_DLL` for the target
- CLI `accsv` (→ `accsv.exe`), link to static lib for portability

**Windows x64 (MSVC):**
```bash
cmake -S . -B build-win64 -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build-win64 --config Release --target accsv accsv_shared
# Copy outputs to dist/windows/x64
```

**Windows x86 (MSVC 32-bit):**
```bash
cmake -S . -B build-win32 -A Win32 -DCMAKE_BUILD_TYPE=Release
cmake --build build-win32 --config Release --target accsv accsv_shared
# Copy outputs to dist/windows/x86
```

**MinGW (optional):**
```bash
x86_64-w64-mingw32-gcc -O3 -Wall -Wextra -pedantic -DACCSV_BUILD_DLL -shared -o accsv.dll accsv.c -lpthread
x86_64-w64-mingw32-gcc -O3 -Wall -Wextra -pedantic -o accsv.exe accsv.c -lpthread
```

### 4.2 WebAssembly (Emscripten)
Export a minimal C API:
- `accsv_parser_new_from_buffer(uint8_t* ptr, size_t len)`
- `accsv_next(...)` (returns slice metadata)
- `accsv_free(...)`
- Optional helpers for line-by-line “view”

**Build (Node + Web):**
```bash
emcc accsv.c -O3 -sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=node,web \
  -sALLOW_MEMORY_GROWTH=1 -sWASM_BIGINT=1 \
  -o dist/wasm/accsv.js
# Ensure accsv.wasm is emitted next to accsv.js
```

Create `dist/wasm/accsv.d.ts` describing the Module factory and exported functions.

### 4.3 Native TS Parser (packages/accsv-ts)
- Implement a zero-alloc streaming parser operating on `Uint8Array`:
  - Reads `SUB` at offset 0 → `hasHeader`
  - Splits by `RS`, within each record splits by `US`
  - Ignores cosmetic newline after `RS`
  - Enforces 128 MiB record limit
- Provide two APIs:
  - `parse(buffer: Uint8Array): Iterable<RecordView>` (pull)
  - `stream(source: AsyncIterable<Uint8Array>): AsyncIterable<RecordView>` (push/stream)
- Output views as `{ fields: Array<{ start: number, end: number }>, fieldCount: number }` plus helpers to decode to strings on demand.
- Tooling:
  ```bash
  pnpm add -D typescript tsup vitest eslint
  pnpm build   # tsup builds ESM + CJS
  pnpm test
  ```

### 4.4 Native JS Parser (packages/accsv-js)
- Transpile the TS impl to JS or write directly in JS for Node (require + import support)
- Provide identical API shape to TS package

---

## 5) Testing Plan (Comprehensive)
### 5.1 Unit Tests (C & TS/JS)
- **Header flag** detection (with/without SUB)
- **Cosmetic newline** handling: `RS` + `LF` vs `RS` + `CRLF`
- **Edge cases**: empty file, zero-field record, record > 128 MiB (expect error), malformed SUB
- **Index**: build → load → seek → slice correctness
- **CSV converter**: quoting rules, embedded commas, CRLF normalization, header → SUB insertion
- **CLI**: argument parsing, exit codes, stderr messages, stdin/stdout piping

### 5.2 Golden Tests
- Prepare canonical `.accsv` fixtures with known `midx` and expected `view`/`count` outputs
- Ensure byte-for-byte matches across platforms

### 5.3 Property-Based Tests
- Generate random tables without US/RS/SUB in fields
- Round-trip: generate → ACCSV bytes → parse → re-emit → parse; assert structural equality
- For TS/JS use `fast-check`; for C use custom generators

### 5.4 Fuzzing
- **C**: build with `-fsanitize=fuzzer,address,undefined` (Clang) and fuzz `accsv_parser_next_record`
- **Harness**: feed random byte streams; ensure no crashes; validate “partial record” vs “EOF” behavior
- **Corpus**: seed with real-world ACCSV, CSV-converted samples, varying line endings

### 5.5 Cross-Impl Equivalence
- Parse the same buffers with **C**, **WASM**, **TS/JS**
- Compare record counts, field counts, field byte slices, and header presence

### 5.6 Performance
- Bench linear scan throughput (MB/s), memory usage
- Large file (≥ 10 GB) scanning (Windows + Linux), offset accuracy vs `midx`

### 5.7 DLL/ABI Tests (Windows)
- Minimal C harness that links against `accsv.dll`; verify symbol visibility and calling convention
- Confirm `__declspec(dllimport)` consumers work; produce `accsv.lib`

---

## 6) CI / CD
Use GitHub Actions (or equivalent) with matrix builds:

**Windows:**
```yaml
strategy:
  matrix:
    arch: [x64, Win32]
steps:
  - uses: actions/checkout@v4
  - uses: ilammy/msvc-dev-cmd@v1
    with: { arch: ${{ matrix.arch }} }
  - run: cmake -S . -B build -A ${{ matrix.arch }} -DCMAKE_BUILD_TYPE=Release
  - run: cmake --build build --config Release --target accsv accsv_shared
  - run: ctest --test-dir build --output-on-failure
  - uses: actions/upload-artifact@v4
    with:
      name: accsv-${{ matrix.arch }}
      path: |
        build/**/accsv.exe
        build/**/accsv.dll
        build/**/accsv.lib
        build/**/accsv.pdb
```

**WASM:**
```yaml
- uses: actions/checkout@v4
- uses: mymindstorm/setup-emsdk@v14
- run: emcc --version
- run: emcc accsv.c -O3 -sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=node,web -o dist/wasm/accsv.js
- uses: actions/upload-artifact@v4
  with:
    name: accsv-wasm
    path: dist/wasm/*
```

**Node/TS:**
```yaml
- uses: actions/setup-node@v4
  with: { node-version: '20' }
- run: pnpm i
- run: pnpm -w build
- run: pnpm -w test -- --run
- uses: actions/upload-artifact@v4
  with:
    name: accsv-ts-js
    path: packages/**/dist/**
```

Add a release job that collects artifacts, computes SHA-256 checksums, and publishes a GitHub Release.

---

## 7) Packaging & Distribution
- **Windows zips**: include `accsv.exe`, `accsv.dll`, `accsv.lib`, README-quickstart, LICENSE, checksums
- **WASM bundle**: `accsv.js`, `accsv.wasm`, `README.md`, `accsv.d.ts`
- **NPM packages**:
  - `@accsv/parser` (TS/JS pure)
  - `@accsv/wasm` (WASM loader + d.ts)
- Ensure package READMEs document the **US/RS/SUB** semantics and **no ACSV misspells**

---

## 8) Acceptance Criteria
A build passes if **all** are true:
1. `accsv.exe` and `accsv.dll` produced for **x64** and **x86**, with working CLI and exported C API.
2. `accsv.wasm` + JS loader function in Node and modern browsers.
3. TS/JS pure parsers parse the same buffers to the same structures as C/WASM.
4. All unit/property/fuzz tests pass; no sanitizer or valgrind errors.
5. Index build/load/validate round-trips; `slice` correctness proven by golden tests.
6. No “ACSV” string present in repo or artifacts; **“ACCSV” only**.
7. Artifacts published with checksums; changelog updated.

---

## 9) Sanity Snippets & CLI Behaviors
- **Count excluding header automatically (if SUB present):**
  ```bash
  accsv count data.accsv  # prints a single integer
  ```
- **View (tabs for US, newlines for RS):**
  ```bash
  accsv view data.accsv | head
  ```
- **Build index:**
  ```bash
  accsv index data.accsv --algo SHA256
  ```
- **Slice (record range, keep header with flag):**
  ```bash
  accsv slice data.accsv 100 200 --add-header | accsv view
  ```
- **CSV to ACCSV (adds SUB if header detected):**
  ```bash
  accsv convert-csv input.csv output.accsv
  ```

---

## 10) Style & Quality Bars
- **C**: C11, `-Wall -Wextra -Wpedantic`, no UB, no unchecked realloc, careful seek/ftell error paths
- **Windows**: Unicode-safe file I/O; fopen/fopen_s handled; CRLF tolerant
- **TS/JS**: No hard-coded encodings; operate on `Uint8Array` and decode only on demand
- **Docs**: Keep spec authoritative; code comments point back to relevant spec sections
- **Perf**: Linear-time scan, O(1) per byte; avoid excessive allocations

---

## 11) Final Output Locations
- `dist/windows/x64/*`
- `dist/windows/x86/*`
- `dist/wasm/*`
- `packages/accsv-ts/dist/*`
- `packages/accsv-js/dist/*`
- `reports/*` (coverage, fuzz, perf, cross-impl matrices)

---

## 12) Non-Goals (For Focus)
- No schema or type inference beyond header detection
- No quoting/escaping layer inside ACCSV (binary data must be pre-encoded by caller)
- No optional delimiters—**US/RS/SUB are fixed**

---

## 13) Rename Audit (ACSV → ACCSV)
- Run textual repo-wide rename; fail CI if string “ACSV” remains:
  ```bash
  git grep -n "ACSV" && exit 1 || true
  ```
- Spellchecker step for docs; ensure code identifiers use **ACCSV** consistently.

---

## 14) Last Step: Refer to the Checklist and Update Completed Items After Each Step
After completing each major step in the implementation process (as outlined in sections 1 through 13), refer back to the separate Mission Checklist provided below. Update it by marking completed items with [x] to track progress. This ensures systematic completion and allows for verification of all requirements before final acceptance.


