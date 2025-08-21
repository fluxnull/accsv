cmake_minimum_required(VERSION 3.10)
project(accsv LANGUAGES C)

# Set the C standard to C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- Compiler Flags ---
if(MSVC)
    add_compile_options(/W3)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- Include Directories ---
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# Add include directory for xxhash
include_directories(/app/xxhash)

# --- Add External Libraries ---

# For blake3, which has its own CMakeLists.txt, we use add_subdirectory.
# This is the correct way to handle complex library dependencies.
add_subdirectory(/app/blake3/c blake3_build)

# For xxhash, which is a simple .c and .h pair, we include the source
# file directly.
set(XXHASH_SOURCES /app/xxhash/xxhash.c)

# --- Source Files ---
set(ACCSV_SOURCES accsv.c)

# --- Static Library ---
add_library(accsv_static STATIC ${ACCSV_SOURCES} ${XXHASH_SOURCES})
# Link static lib against the blake3 target
target_link_libraries(accsv_static PRIVATE blake3)

# --- Shared Library (DLL for Windows) ---
add_library(accsv_shared SHARED ${ACCSV_SOURCES} ${XXHASH_SOURCES})
target_link_libraries(accsv_shared PRIVATE blake3)

if(WIN32)
    target_compile_definitions(accsv_shared PRIVATE ACCSV_BUILD_DLL)
    set_target_properties(accsv_shared PROPERTIES OUTPUT_NAME accsv)
endif()

# --- CLI Executable ---
add_executable(accsv ${ACCSV_SOURCES} ${XXHASH_SOURCES})
# Link executable against the blake3 library target
target_link_libraries(accsv PRIVATE blake3)

# --- PThreads for parallel processing ---
find_package(Threads)
if(THREADS_FOUND)
    message(STATUS "Threads found, enabling pthreads support.")
    target_compile_definitions(accsv PRIVATE HAVE_PTHREADS)
    target_link_libraries(accsv PRIVATE Threads::Threads)
    target_compile_definitions(accsv_static PRIVATE HAVE_PTHREADS)
    target_link_libraries(accsv_static PRIVATE Threads::Threads)
    target_compile_definitions(accsv_shared PRIVATE HAVE_PTHREADS)
    target_link_libraries(accsv_shared PRIVATE Threads::Threads)
else()
    message(STATUS "Threads not found, pthreads support will be disabled.")
endif()

# --- Installation (Optional, for packaging) ---
install(TARGETS accsv accsv_shared accsv_static
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(FILES accsv.h DESTINATION include)

# --- Testing with CTest ---
enable_testing()
# add_executable(run_tests tests/test_main.c)
# target_link_libraries(run_tests PRIVATE accsv_static)
# add_test(NAME unit_tests COMMAND run_tests)
