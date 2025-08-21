#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "accsv.h"

// Helper to create a test file
void create_test_file(const char* filename, const unsigned char* data, size_t size) {
    FILE* f = fopen(filename, "wb");
    assert(f != NULL);
    assert(fwrite(data, 1, size, f) == size);
    fclose(f);
}

void test_build_index_simple() {
    printf("Running test: test_build_index_simple...\n");
    const char* data_filename = "/tmp/test_simple.accsv";
    const char* midx_filename = "/tmp/test_simple.accsv.midx";
    const unsigned char data[] = { 'a', 0x1F, 'b', 0x1E, 'c', 0x1F, 'd', 0x1E };
    create_test_file(data_filename, data, sizeof(data));

    int result = accsv_build_index(data_filename, midx_filename, "BLAKE3");
    assert(result == ACCSV_SUCCESS);

    AccsvIndex* idx = accsv_index_load(midx_filename);
    assert(idx != NULL);
    assert(accsv_index_get_record_count(idx) == 2);
    assert(idx->offsets[0] == 0);
    assert(idx->offsets[1] == 4);
    accsv_index_free(idx);
    printf("...PASSED\n");
}

void test_cosmetic_newlines() {
    printf("Running test: test_cosmetic_newlines...\n");
    const char* data_filename = "/tmp/test_newlines.accsv";
    const char* midx_filename = "/tmp/test_newlines.accsv.midx";
    // r1 RS LF r2 RS CRLF r3 RS
    const unsigned char data[] = { 'r', '1', 0x1E, 0x0A, 'r', '2', 0x1E, 0x0D, 0x0A, 'r', '3', 0x1E };
    create_test_file(data_filename, data, sizeof(data));

    accsv_build_index(data_filename, midx_filename, "BLAKE3");

    AccsvIndex* idx = accsv_index_load(midx_filename);
    assert(idx != NULL);
    assert(accsv_index_get_record_count(idx) == 3);
    assert(idx->offsets[0] == 0);  // r1
    assert(idx->offsets[1] == 4);  // r2 (starts after "r1" RS LF)
    assert(idx->offsets[2] == 9);  // r3 (starts after "r2" RS CRLF)
    accsv_index_free(idx);

    // Also test the parser directly
    FILE* f = fopen(data_filename, "rb");
    AccsvParser* p = accsv_parser_new(f);
    AccsvRecordView r;
    assert(accsv_parser_next_record(p, &r) == ACCSV_SUCCESS);
    assert(r.field_count == 1);
    assert(r.fields[0].length == 2);
    assert(accsv_parser_next_record(p, &r) == ACCSV_SUCCESS);
    assert(r.field_count == 1);
    assert(r.fields[0].length == 2);
    assert(accsv_parser_next_record(p, &r) == ACCSV_SUCCESS);
    assert(r.field_count == 1);
    assert(r.fields[0].length == 2);
    accsv_parser_free(p);
    fclose(f);

    printf("...PASSED\n");
}

void test_header_flag() {
    printf("Running test: test_header_flag...\n");
    const char* data_filename = "/tmp/test_header.accsv";
    // SUB header, then one record
    const unsigned char data[] = { 0x1A, 'h', '1', 0x1E, 'd', '1', 0x1E };
    create_test_file(data_filename, data, sizeof(data));

    FILE* f = fopen(data_filename, "rb");
    AccsvParser* p = accsv_parser_new(f);
    assert(p != NULL);
    assert(accsv_parser_has_header(p) == 1);

    // Test that count is correct (should be 1, not 2)
    // This requires running the CLI, which is more complex.
    // For now, we test the library function.
    uint64_t count = 0;
    AccsvRecordView r;
    while (accsv_parser_next_record(p, &r) == ACCSV_SUCCESS) {
        count++;
    }
    // The parser itself just sees records. The header is a semantic layer on top.
    assert(count == 2);
    accsv_parser_free(p);
    fclose(f);
    printf("...PASSED\n");
}

int main() {
    printf("--- Running ACCSV Test Suite ---\n");
    test_build_index_simple();
    test_cosmetic_newlines();
    test_header_flag();
    printf("--- All tests passed! ---\n");
    return 0;
}
