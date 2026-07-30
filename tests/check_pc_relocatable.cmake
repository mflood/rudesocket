# Asserts the generated pkg-config file expresses its prefix relative to
# ${pcfiledir} rather than baking in the configure-time CMAKE_INSTALL_PREFIX.
#
# Before 5.1.1 the .pc read "prefix=/usr/local" no matter where the package was
# actually installed, so the README's own
#   cmake --install build --prefix <dir> && pkg-config --cflags rudesocket
# flow produced -I/usr/local/include and the compile failed with
# "'rude/socket.h' file not found" -- or, worse, silently found an older copy.

if(NOT EXISTS "${PC_FILE}")
    message(FATAL_ERROR "pkg-config file not generated: ${PC_FILE}")
endif()

file(STRINGS "${PC_FILE}" pc_lines REGEX "^prefix=")

if(NOT pc_lines)
    message(FATAL_ERROR "no prefix= line in ${PC_FILE}")
endif()

list(GET pc_lines 0 prefix_line)

if(NOT prefix_line MATCHES "^prefix=\\\${pcfiledir}/")
    message(FATAL_ERROR
        "pkg-config prefix is not relocatable: '${prefix_line}'\n"
        "Expected it to be expressed relative to \${pcfiledir}.")
endif()

message(STATUS "pkg-config prefix is relocatable: ${prefix_line}")
