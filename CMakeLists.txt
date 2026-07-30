cmake_minimum_required(VERSION 3.16)

project(rudesocket
    VERSION 1.3.0
    DESCRIPTION "C++ TCP socket client library with SSL/TLS, SOCKS4/5, and HTTP proxy support"
    HOMEPAGE_URL "https://github.com/mflood/rudesocket"
    LANGUAGES CXX)

include(GNUInstallDirs)
include(CTest)

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
else()
    set(PC_REQUIRES_PRIVATE "")
endif()
configure_file(rudesocket.pc.in "${CMAKE_CURRENT_BINARY_DIR}/rudesocket.pc" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/rudesocket.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

# ---- Tests & examples ----

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(RUDESOCKET_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
