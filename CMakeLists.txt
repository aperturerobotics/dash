cmake_minimum_required(VERSION 3.10)
project(dash C)

# Source files from the original Makefile.am.
set(DASH_SOURCES
    src/alias.c
    src/arith_yacc.c
    src/arith_yylex.c
    src/cd.c
    src/error.c
    src/eval.c
    src/exec.c
    src/expand.c
    src/histedit.c
    src/input.c
    src/jobs.c
    src/mail.c
    src/main.c
    src/memalloc.c
    src/miscbltin.c
    src/mystring.c
    src/options.c
    src/parser.c
    src/redir.c
    src/show.c
    src/trap.c
    src/output.c
    src/system.c
    src/var.c
    src/bltin/printf.c
    src/bltin/test.c
    src/bltin/times.c
)

# Pre-generated source files (run native build first to generate).
set(DASH_GENERATED
    src/builtins.c
    src/init.c
    src/nodes.c
    src/signames.c
    src/syntax.c
)

# Common compile definitions.
set(DASH_DEFINES BSD=1 SHELL)

# Standard (native) build: command model with main().
if(NOT CMAKE_SYSTEM_NAME STREQUAL "WASI")
    add_executable(dash ${DASH_SOURCES} ${DASH_GENERATED})
    target_compile_definitions(dash PRIVATE ${DASH_DEFINES})
    target_include_directories(dash PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}
    )
    target_compile_options(dash PRIVATE -Wall -include config.h)
endif()

# WASI reactor build.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
    option(DASH_WASI_REACTOR
        "Build WASI reactor (exports library functions, no _start)" OFF)

    if(DASH_WASI_REACTOR)
        add_executable(dash_wasi
            ${DASH_SOURCES}
            ${DASH_GENERATED}
            src/reactor.c
            src/wasi-stubs.c
        )
        set_target_properties(dash_wasi PROPERTIES
            OUTPUT_NAME "dash"
            SUFFIX ".wasm"
        )
        target_compile_definitions(dash_wasi PRIVATE
            ${DASH_DEFINES}
            SMALL=1
            JOBS=0
            _WASI_EMULATED_SIGNAL
            _WASI_EMULATED_PROCESS_CLOCKS
            _WASI_EMULATED_GETPID
        )
        # wasi-compat MUST come before src so our stub sys/ headers
        # shadow the missing WASI ones (sys/wait.h, etc.).
        target_include_directories(dash_wasi PRIVATE
            ${CMAKE_SOURCE_DIR}/wasi-compat
            ${CMAKE_SOURCE_DIR}/src
        )
        target_compile_options(dash_wasi PRIVATE
            -Wall
            -include ${CMAKE_SOURCE_DIR}/config-wasi.h
            # Enable setjmp/longjmp via WASM Exception Handling proposal.
            -mllvm -wasm-enable-sjlj
            # Suppress warnings from dash's older C style.
            -Wno-string-plus-int
            -Wno-unused-value
            -Wno-unused-parameter
            -Wno-missing-field-initializers
            -Wno-parentheses
        )
        target_link_options(dash_wasi PRIVATE
            -mexec-model=reactor
            -Wl,--export-dynamic
            -Wl,--export=malloc
            -Wl,--export=free
            -Wl,--export=realloc
            -Wl,--export=calloc
            -lsetjmp
            -lwasi-emulated-signal
            -lwasi-emulated-process-clocks
            -lwasi-emulated-getpid
        )
    endif()
endif()
