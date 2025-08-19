#### Section 0: Mission Instructions (TL;DR)
- [ ] Fully parse and ingest the entire content of **ACCSV Specification Document v5.0.md**, extracting every normative requirement including file formats, separators, error handling, and philosophy.
- [ ] Complete all placeholder functions in **accsv.c**, specifically implementing parallel processing, index building/loading/validation, CSV conversion logic, record appending, and the full CLI main function with argument parsing.
- [ ] Verify that **accsv.h** function signatures exactly match the final implemented code in **accsv.c**, and add Windows DLL export macros to all public API functions.
- [ ] Implement and finish all CLI commands including `index` with digest computation, `count` with header exclusion, `view` with human-readable rendering, `slice` with range extraction and optional header addition, and `convert-csv` with quoting and header detection.
- [ ] Build and produce Windows x64 and x86 binaries, including `accsv.exe`, `accsv.dll`, and corresponding `.lib` import libraries for both architectures.
- [ ] Build the WebAssembly target for Node and browser environments, including the `accsv.wasm` binary, a thin JS loader script, and comprehensive TypeScript typings for the exported API.
- [ ] Implement pure native TypeScript and JavaScript parsers that are fully spec-compliant, providing streaming and buffer-based APIs with zero-copy views and strict error handling.
- [ ] Create and run exhaustive tests covering unit, golden, property-based, fuzzing, and performance aspects, while verifying full equivalence across C, WASM, TS, and JS implementations.
- [ ] Ship all final artifacts through CI pipelines, including release notes, SHA-256 checksums for every file, and ensure no occurrences of “ACSV” spelling in any code, docs, or logs.

#### Section 1: Inputs & Ground Truth
- [ ] Review and enforce all primary spec details from `ACCSV Specification Document v5.0.md`, including byte-level rules for US, RS, SUB, cosmetic newlines, encoding agnosticism, max record size, and edge case behaviors.
- [ ] Validate and implement all index file requirements for `.accsv.midx`, covering hybrid text/binary structure, meta keys, binary header with magic number, version, reserved bytes, record count, and offsets array in little-endian format.

#### Section 2: Deliverables
- [ ] Produce Windows x64 deliverables: `accsv.exe`, `accsv.dll`, `accsv.lib`, and PDB files, ensuring version reporting and ACCSV branding consistency.
- [ ] Produce Windows x86 deliverables: `accsv.exe`, `accsv.dll`, `accsv.lib`, and PDB files, with identical version and branding checks.
- [ ] Produce WebAssembly deliverables: `accsv.wasm`, `accsv.js` loader with MODULARIZE, and `accsv.d.ts` for TypeScript integration.
- [ ] Produce native TS parser deliverables: `packages/accsv-ts/dist/index.js` and `index.d.ts`, with streaming and buffer APIs matching spec parity.
- [ ] Produce native JS parser deliverables: `packages/accsv-js/dist/index.js` in ESM and CJS formats, with identical API shape and spec conformance.
- [ ] Generate all test and report deliverables: unit/property/fuzz/golden/performance results, equivalence matrices, coverage reports, memory logs, and documented fuzz crashers.
- [ ] Set up CI pipelines for all platforms and produce signed artifacts with SHA-256 checksums, CHANGELOG entries, and per-target README sections.

#### Section 3: Implementation Tasks
- [ ] Implement all missing functions in **accsv.c** as listed, ensuring match to **accsv.h** and full spec compliance for parallel, indexing, seeking, building, validation, CSV conversion, appending, and CLI main.
- [ ] Enforce all notes and constraints in implementation, including memchr usage, zero-copy views, buffer caps, cosmetic newline handling, SUB detection, and CLI error/piping support.
- [ ] For index builder, compute record count and offsets, include required meta keys, support algo choices with digest computation, and integrate local xxhash/blake3 sources if needed for additional algos.
- [ ] For CSV converter, build a full RFC-4180 state machine handling quoting, embedded commas, line endings, and automatic SUB prepending on header detection.
- [ ] Add and apply Windows DLL export macros in **accsv.h** to all public symbols, maintaining stable C ABI without mangling.
- [ ] Ensure CLI UX consistency with exact help output, version printing, and view rendering as specified.

#### Section 4: Build System & Commands
- [ ] Create and configure CMakeLists.txt to build static lib, shared lib with DLL define, and CLI executable linked to static for portability.
- [ ] Execute and verify Windows x64 build commands using MSVC, copying outputs to designated dist folder.
- [ ] Execute and verify Windows x86 build commands using MSVC, copying outputs to designated dist folder.
- [ ] Optionally test MinGW build commands for cross-compatibility.
- [ ] Export minimal C API for WASM and execute Emscripten build commands, ensuring wasm and js outputs with memory growth and bigint support.
- [ ] Create TypeScript declarations for WASM module factory and functions.
- [ ] Implement zero-alloc TS parser on Uint8Array with header detection, splitting logic, newline ignoring, and size enforcement, providing pull and push APIs with view outputs.
- [ ] Transpile or implement JS parser with identical features and API shape as TS version.

#### Section 5: Testing Plan (Comprehensive)
- [ ] Develop and run unit tests for header detection, newline handling, edge cases, index operations, CSV conversion, and CLI behaviors in both C and TS/JS.
- [ ] Prepare and execute golden tests with fixtures, verifying outputs and cross-platform byte matches.
- [ ] Implement property-based tests with random generation, round-trip assertions, using fast-check in TS/JS and custom in C.
- [ ] Set up fuzzing for C with sanitizers, harness random streams, and build corpus from real samples.
- [ ] Perform cross-implementation equivalence testing, comparing counts, fields, slices, and headers across all parsers.
- [ ] Run performance benchmarks on throughput, memory, and large files, validating offsets against midx.
- [ ] Create DLL/ABI tests with harness linking to dll, verifying symbols and conventions.

#### Section 6: CI / CD
- [ ] Configure GitHub Actions matrix for Windows builds, including checkout, MSVC setup, cmake build/test, and artifact upload.
- [ ] Configure GitHub Actions for WASM builds, including emsdk setup, emcc compilation, and artifact upload.
- [ ] Configure GitHub Actions for Node/TS builds, including node setup, pnpm install/build/test, and artifact upload.
- [ ] Add release job to collect artifacts, compute checksums, and publish GitHub Release.

#### Section 7: Packaging & Distribution
- [ ] Package Windows zips with exe, dll, lib, README, LICENSE, and checksums.
- [ ] Bundle WASM with js, wasm, README, and d.ts.
- [ ] Publish NPM packages for pure parser and WASM loader, with READMEs documenting semantics without misspellings.

#### Section 8: Acceptance Criteria
- [ ] Verify all acceptance points: binaries produced with working CLI/API, WASM functioning in environments, parser equivalence, test passes without errors, round-trips, no ACSV strings, and published artifacts with checksums/changelog.

#### Section 9: Sanity Snippets & CLI Behaviors
- [ ] Test and confirm count command excludes headers automatically.
- [ ] Test and confirm view command renders correctly with tabs and newlines.
- [ ] Test and confirm index build with algo option.
- [ ] Test and confirm slice with range and header flag.
- [ ] Test and confirm CSV to ACCSV conversion with SUB addition.

#### Section 10: Style & Quality Bars
- [ ] Adhere to C11 standards with full warnings, no undefined behavior, checked allocations, and error paths.
- [ ] Ensure Windows-specific quality: Unicode I/O, safe fopen, CRLF tolerance.
- [ ] Follow TS/JS bars: Uint8Array operations, on-demand decoding.
- [ ] Maintain doc quality with spec references in comments.
- [ ] Achieve perf goals: linear time, constant per-byte, minimal allocations.

#### Section 11: Final Output Locations
- [ ] Place all outputs in specified dist and packages folders, including reports for coverage, fuzz, perf, and matrices.

#### Section 12: Non-Goals (For Focus)
- [ ] Confirm avoidance of non-goals: no schema inference, no internal quoting, fixed delimiters.

#### Section 13: Rename Audit (ACSV → ACCSV)
- [ ] Run repo-wide grep for ACSV and fail CI if found.
- [ ] Implement spellchecker in docs and ensure consistent ACCSV identifiers.

#### Section 14: Last Step
- [ ] After each major step, refer to this checklist and mark items as completed with [x].

---

### That’s it. Execute in order, keep the spec as law, and ship clean artifacts.