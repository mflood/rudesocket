find_package(Threads REQUIRED)

add_executable(test_echo test_echo.cpp)
target_link_libraries(test_echo PRIVATE rudesocket::rudesocket Threads::Threads)
add_test(NAME echo COMMAND test_echo)

add_executable(test_timeout test_timeout.cpp)
target_link_libraries(test_timeout PRIVATE rudesocket::rudesocket Threads::Threads)
add_test(NAME timeout COMMAND test_timeout)

add_executable(test_contract test_contract.cpp)
target_link_libraries(test_contract PRIVATE rudesocket::rudesocket Threads::Threads)
add_test(NAME contract COMMAND test_contract)

add_executable(test_readline_eof test_readline_eof.cpp)
target_link_libraries(test_readline_eof PRIVATE rudesocket::rudesocket Threads::Threads)
add_test(NAME readline_eof COMMAND test_readline_eof)

set(rudesocket_tests echo timeout readline_eof contract)

if(RUDESOCKET_SSL_ENABLED)
    add_executable(test_ssl test_ssl.cpp)
    target_link_libraries(test_ssl PRIVATE rudesocket::rudesocket Threads::Threads)
    add_test(NAME ssl COMMAND test_ssl)
    list(APPEND rudesocket_tests ssl)

    add_executable(test_ssl_timeout test_ssl_timeout.cpp)
    target_link_libraries(test_ssl_timeout PRIVATE rudesocket::rudesocket Threads::Threads)
    add_test(NAME ssl_timeout COMMAND test_ssl_timeout)
    list(APPEND rudesocket_tests ssl_timeout)
endif()

# Each test runs in its own scratch dir so fixtures (certs, logs) don't collide.
set_tests_properties(${rudesocket_tests} PROPERTIES TIMEOUT 60)
foreach(t ${rudesocket_tests})
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/run-${t}")
    set_tests_properties(${t} PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/run-${t}")
endforeach()

# Guards the 1.3.1 pkg-config fix: rudesocket.pc must not hardcode the
# configure-time prefix, or "cmake --install --prefix <dir>" ships a .pc
# pointing somewhere else entirely (worst case, an older system copy).
# Guards that pkg-config consumers can detect SSL the same way CMake
# consumers can (1.4.1).
add_test(NAME pkgconfig_ssl
    COMMAND "${CMAKE_COMMAND}"
        "-DPC_FILE=${CMAKE_BINARY_DIR}/rudesocket.pc"
        "-DEXPECT_SSL=$<BOOL:${RUDESOCKET_SSL_ENABLED}>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_pc_ssl.cmake")

add_test(NAME pkgconfig_relocatable
    COMMAND "${CMAKE_COMMAND}"
        "-DPC_FILE=${CMAKE_BINARY_DIR}/rudesocket.pc"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_pc_relocatable.cmake")
