cmake_minimum_required(VERSION 3.16)

project(rudesocket
    VERSION 1.6.0
    DESCRIPTION "C++ TCP socket client library with SSL/TLS, SOCKS4/5, and HTTP proxy support"
    HOMEPAGE_URL "https://github.com/mflood/rudesocket"
    LANGUAGES CXX)

# PROJECT_IS_TOP_LEVEL is CMake 3.21+; this project still supports 3.16, and
# an undefined variable would read as false and silently drop the tests.
if(NOT DEFINED PROJECT_IS_TOP_LEVEL)
    if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        set(PROJECT_IS_TOP_LEVEL TRUE)
    else()
        set(PROJECT_IS_TOP_LEVEL FALSE)
    endif()
endif()

include(GNUInstallDirs)
# Only offer tests and examples when this is the top-level project.
# Without the guard, a FetchContent consumer inherits our demo and
# test_* targets into their own build.
if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
endif()

option(RUDESOCKET_BUILD_EXAMPLES "Build example programs" ON)
option(RUDESOCKET_WITH_SSL "Build with SSL/TLS support (requires OpenSSL)" ON)

# SSL is optional, mirroring the old "./configure --with-openssl" mechanism:
# when disabled (or when OpenSSL is missing), the library still builds and
# connectSSL() fails gracefully with a clear getError() message.
set(RUDESOCKET_SSL_ENABLED OFF)
if(RUDESOCKET_WITH_SSL)
    find_package(OpenSSL)
    if(OPENSSL_FOUND)
        set(RUDESOCKET_SSL_ENABLED ON)
    else()
        message(WARNING "RUDESOCKET_WITH_SSL=ON but OpenSSL was not found; building without SSL support")
    endif()
endif()

add_library(rudesocket
    src/rude_socketimpl.cpp
    src/socket.cpp
    src/socket_comm.cpp
    src/socket_comm_normal.cpp
    src/socket_connect.cpp
    src/socket_connect_normal.cpp
    src/socket_connect_proxy.cpp
    src/socket_connect_socks4.cpp
    src/socket_connect_socks5.cpp
    src/socket_connect_tunnel.cpp
    src/socket_tcpclient.cpp)

if(RUDESOCKET_SSL_ENABLED)
    target_sources(rudesocket PRIVATE src/socket_comm_ssh.cpp)
    # USING_OPENSSL is the same feature macro the autotools build used;
    # RUDESOCKET_WITH_SSL is exported so consumers (and the demo) can detect SSL.
    target_compile_definitions(rudesocket PRIVATE USING_OPENSSL)
    target_compile_definitions(rudesocket PUBLIC RUDESOCKET_WITH_SSL)
    target_link_libraries(rudesocket PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

add_library(rudesocket::rudesocket ALIAS rudesocket)

target_compile_features(rudesocket PUBLIC cxx_std_17)

# Public header is consumed as <rude/socket.h>; stage it under that path
# so in-tree consumers see the same layout as installed ones.
configure_file(src/socket.h "${CMAKE_CURRENT_BINARY_DIR}/include/rude/socket.h" COPYONLY)
target_include_directories(rudesocket PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

# Shared-library version follows the old libtool -version-info 1:2:0 lineage.
set_target_properties(rudesocket PROPERTIES
    VERSION 1.0.2
    SOVERSION 1
    POSITION_INDEPENDENT_CODE ON
    WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(WIN32)
    target_link_libraries(rudesocket PUBLIC ws2_32)
endif()

if(MSVC)
    target_compile_options(rudesocket PRIVATE /W4)
else()
    target_compile_options(rudesocket PRIVATE -Wall -Wextra)
endif()

# ---- Install ----

install(TARGETS rudesocket
    EXPORT rudesocket-targets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(FILES src/socket.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/rude")

install(FILES man3/rudesocket.3
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man3")

install(EXPORT rudesocket-targets
    FILE rudesocket-targets.cmake
    NAMESPACE rudesocket::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/rudesocket")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/rudesocket-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
if(RUDESOCKET_SSL_ENABLED)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/rudesocket-config.cmake"
        "include(CMakeFindDependencyMacro)\nfind_dependency(OpenSSL)\ninclude(\"\${CMAKE_CURRENT_LIST_DIR}/rudesocket-targets.cmake\")\n")
else()
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/rudesocket-config.cmake"
        "include(\"\${CMAKE_CURRENT_LIST_DIR}/rudesocket-targets.cmake\")\n")
endif()
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/rudesocket-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/rudesocket-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/rudesocket")

if(RUDESOCKET_SSL_ENABLED)
    set(PC_REQUIRES_PRIVATE "Requires.private: openssl")
    # CMake consumers get RUDESOCKET_WITH_SSL from the exported target; give
    # pkg-config consumers the same feature macro so they can detect SSL too.
    set(PC_SSL_CFLAGS " -DRUDESOCKET_WITH_SSL")
else()
    set(PC_REQUIRES_PRIVATE "")
    set(PC_SSL_CFLAGS "")
endif()
# The .pc file must not bake in the configure-time CMAKE_INSTALL_PREFIX: that
# breaks "cmake --install --prefix <dir>", which is what the README teaches.
# Express prefix relative to ${pcfiledir} instead, so the file stays correct
# wherever it lands.  Walk back up one "../" per component of
# <libdir>/pkgconfig -- computed by hand rather than with file(RELATIVE_PATH),
# which requires absolute arguments and rejects "/" as a root on Windows.
# Falls back to the absolute prefix if the caller set an absolute
# CMAKE_INSTALL_LIBDIR, where a relative path is not meaningful.
if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
    set(RUDESOCKET_PC_PREFIX "${CMAKE_INSTALL_PREFIX}")
else()
    file(TO_CMAKE_PATH "${CMAKE_INSTALL_LIBDIR}/pkgconfig" RUDESOCKET_PC_SUBDIR)
    string(REGEX REPLACE "/+" "/" RUDESOCKET_PC_SUBDIR "${RUDESOCKET_PC_SUBDIR}")
    string(REGEX REPLACE "^/+|/+$" "" RUDESOCKET_PC_SUBDIR "${RUDESOCKET_PC_SUBDIR}")
    string(REPLACE "/" ";" RUDESOCKET_PC_PARTS "${RUDESOCKET_PC_SUBDIR}")
    set(RUDESOCKET_PC_REL_PREFIX "")
    foreach(part IN LISTS RUDESOCKET_PC_PARTS)
        if(NOT part STREQUAL "" AND NOT part STREQUAL ".")
            string(APPEND RUDESOCKET_PC_REL_PREFIX "../")
        endif()
    endforeach()
    if(RUDESOCKET_PC_REL_PREFIX STREQUAL "")
        set(RUDESOCKET_PC_REL_PREFIX "./")
    endif()
    set(RUDESOCKET_PC_PREFIX "\${pcfiledir}/${RUDESOCKET_PC_REL_PREFIX}")
endif()

configure_file(rudesocket.pc.in "${CMAKE_CURRENT_BINARY_DIR}/rudesocket.pc" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/rudesocket.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

# ---- Tests & examples ----

if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(PROJECT_IS_TOP_LEVEL AND RUDESOCKET_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
