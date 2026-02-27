cmake_minimum_required(VERSION 3.16)
project(yaps
        VERSION 0.1.0
        LANGUAGES C
)

# -------------------------
# Configurable options
# -------------------------
option(BUILD_SHARED_LIBS       "Build shared libraries" OFF)
option(ENABLE_PCAP             "Enable libpcap support (packet capture)" OFF)
option(ENABLE_LIBNET           "Enable libnet support (packet crafting)" OFF)
option(ENABLE_PLUGINS          "Enable plugin support (dlopen)" ON)
option(ENABLE_TESTS            "Build unit/integration tests" OFF)
option(ENABLE_SANITIZERS       "Enable Address/Undefined sanitizers (debug builds)" OFF)
option(COVERAGE                "Enable code coverage (uses gcc/clang flags)" OFF)

# -------------------------
# Basic settings
# -------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# Useful compile flags
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# -------------------------
# Source files
# -------------------------
# You can prefer explicit lists for reproducibility; use glob for convenience
file(GLOB_RECURSE YAPS_SOURCES
        "${CMAKE_SOURCE_DIR}/src/*.c"
        "${CMAKE_SOURCE_DIR}/src/**/*.c"
)

# Public include dir
include_directories("${CMAKE_SOURCE_DIR}/include")
include_directories("${CMAKE_SOURCE_DIR}/src")

# -------------------------
# Find dependencies
# -------------------------
find_package(Threads REQUIRED)
find_program(PKG_CONFIG_EXECUTABLE pkg-config)

if(ENABLE_PCAP)
    if(NOT PKG_CONFIG_EXECUTABLE)
        message(FATAL_ERROR "pkg-config not found; required to find libpcap. Install pkg-config or disable ENABLE_PCAP.")
    endif()
    execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --exists "libpcap"
            RESULT_VARIABLE PCAP_PKG_EXISTS)
    if(PCAP_PKG_EXISTS EQUAL 0)
        pkg_check_modules(PCAP REQUIRED libpcap)
        include_directories(${PCAP_INCLUDE_DIRS})
        link_directories(${PCAP_LIBRARY_DIRS})
        add_compile_definitions(HAVE_PCAP=1)
        set(EXTRA_LIBS ${EXTRA_LIBS} ${PCAP_LIBRARIES})
    else()
        message(WARNING "libpcap not found via pkg-config; building without pcap support.")
        set(ENABLE_PCAP OFF)
    endif()
endif()

if(ENABLE_LIBNET)
    if(NOT PKG_CONFIG_EXECUTABLE)
        message(FATAL_ERROR "pkg-config not found; required to find libnet. Install pkg-config or disable ENABLE_LIBNET.")
    endif()
    execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --exists "libnet"
            RESULT_VARIABLE LIBNET_PKG_EXISTS)
    if(LIBNET_PKG_EXISTS EQUAL 0)
        pkg_check_modules(LIBNET REQUIRED libnet)
        include_directories(${LIBNET_INCLUDE_DIRS})
        link_directories(${LIBNET_LIBRARY_DIRS})
        add_compile_definitions(HAVE_LIBNET=1)
        set(EXTRA_LIBS ${EXTRA_LIBS} ${LIBNET_LIBRARIES})
    else()
        message(WARNING "libnet not found via pkg-config; building without libnet support.")
        set(ENABLE_LIBNET OFF)
    endif()
endif()

# Plugins use dl on POSIX systems
if(ENABLE_PLUGINS AND NOT WIN32)
    find_library(DL_LIB dl)
    if(DL_LIB)
        set(EXTRA_LIBS ${EXTRA_LIBS} ${DL_LIB})
    else()
        message(WARNING "libdl not found; plugin support may be limited.")
    endif()
    add_compile_definitions(ENABLE_PLUGINS=1)
endif()

# -------------------------
# Sanitizers & coverage (for Debug builds)
# -------------------------
if(ENABLE_SANITIZERS)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(WARNING "Sanitizers are typically supported with GCC/Clang only.")
    else()
        add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address -fsanitize=undefined)
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O1 -g")
    endif()
endif()

if(COVERAGE)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(WARNING "Coverage flags are for GCC/Clang only.")
    else()
        add_compile_options(--coverage -O0 -g)
        add_link_options(--coverage)
    endif()
endif()

# -------------------------
# Build target: yaps (executable)
# -------------------------
add_executable(yaps ${YAPS_SOURCES})

target_include_directories(yaps PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(yaps PRIVATE ${EXTRA_LIBS} Threads::Threads)

# Define version header macro
target_compile_definitions(yaps PRIVATE YAPS_VERSION="${PROJECT_VERSION}")

# Install rule
install(TARGETS yaps
        RUNTIME DESTINATION bin
)

## -------------------------
## Testing (basic integration with CTest)
## -------------------------
#if(ENABLE_TESTS)
#    enable_testing()
#    # Prefer a tests CMakeLists or small unit test driver
#    add_subdirectory(tests)
#endif()

# -------------------------
# Packaging helpers
# -------------------------
include(CPack)
set(CPACK_PACKAGE_NAME "yaps")
set(CPACK_PACKAGE_VENDOR "yaps project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "yaps - lightweight network scanner (nmap-like)")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "TGZ;ZIP")

# -------------------------
# Utility targets
# -------------------------
# print-config: quick summary of options
add_custom_target(print-config
        COMMAND ${CMAKE_COMMAND} -E echo "Configuration:"
        COMMAND ${CMAKE_COMMAND} -E echo "  BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        COMMAND ${CMAKE_COMMAND} -E echo "  ENABLE_PCAP=${ENABLE_PCAP}"
        COMMAND ${CMAKE_COMMAND} -E echo "  ENABLE_LIBNET=${ENABLE_LIBNET}"
        COMMAND ${CMAKE_COMMAND} -E echo "  ENABLE_PLUGINS=${ENABLE_PLUGINS}"
        COMMENT "Show build configuration"
)

# clang-format/astyle targets (if present)
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_C_AND_H
            "${CMAKE_SOURCE_DIR}/src/*.c" "${CMAKE_SOURCE_DIR}/src/**/*.c"
            "${CMAKE_SOURCE_DIR}/include/*.h" "${CMAKE_SOURCE_DIR}/src/**/*.h"
    )
    add_custom_target(format
            COMMAND ${CLANG_FORMAT} -i ${ALL_C_AND_H}
            COMMENT "Run clang-format on source files"
    )
endif()

# -------------------------
# Helpful messages
# -------------------------
message(STATUS "Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "PCAP enabled: ${ENABLE_PCAP}")
message(STATUS "LIBNET enabled: ${ENABLE_LIBNET}")
message(STATUS "PLUGINS enabled: ${ENABLE_PLUGINS}")
message(STATUS "SANITIZERS enabled: ${ENABLE_SANITIZERS}")