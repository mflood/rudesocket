# Asserts the generated pkg-config file advertises SSL the same way the CMake
# target does. CMake consumers get RUDESOCKET_WITH_SSL from the exported
# target; before 1.4.1 pkg-config consumers got no equivalent, so they had no
# way to detect whether the library they linked supported TLS.

if(NOT EXISTS "${PC_FILE}")
    message(FATAL_ERROR "pkg-config file not generated: ${PC_FILE}")
endif()

file(STRINGS "${PC_FILE}" cflags_lines REGEX "^Cflags:")
if(NOT cflags_lines)
    message(FATAL_ERROR "no Cflags line in ${PC_FILE}")
endif()
list(GET cflags_lines 0 cflags)

file(STRINGS "${PC_FILE}" requires_lines REGEX "^Requires.private:")

if(EXPECT_SSL)
    if(NOT cflags MATCHES "-DRUDESOCKET_WITH_SSL")
        message(FATAL_ERROR
            "SSL is enabled but the .pc does not define RUDESOCKET_WITH_SSL: '${cflags}'")
    endif()
    if(NOT requires_lines)
        message(FATAL_ERROR "SSL is enabled but the .pc has no Requires.private for openssl")
    endif()
    message(STATUS "pkg-config advertises SSL: ${cflags}")
else()
    if(cflags MATCHES "-DRUDESOCKET_WITH_SSL")
        message(FATAL_ERROR
            "SSL is disabled but the .pc still defines RUDESOCKET_WITH_SSL: '${cflags}'")
    endif()
    message(STATUS "pkg-config correctly advertises no SSL")
endif()
