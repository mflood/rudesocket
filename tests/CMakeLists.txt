find_package(Threads REQUIRED)

add_executable(test_echo test_echo.cpp)
target_link_libraries(test_echo PRIVATE rudesocket::rudesocket Threads::Threads)
add_test(NAME echo COMMAND test_echo)

add_executable(test_timeout test_timeout.cpp)
target_link_libraries(test_timeout PRIVATE rudesocket::rudesocket Threads::Threads)
add_test(NAME timeout COMMAND test_timeout)

set(rudesocket_tests echo timeout)

if(RUDESOCKET_SSL_ENABLED)
    add_executable(test_ssl test_ssl.cpp)
    target_link_libraries(test_ssl PRIVATE rudesocket::rudesocket Threads::Threads)
    add_test(NAME ssl COMMAND test_ssl)
    list(APPEND rudesocket_tests ssl)
endif()

# Each test runs in its own scratch dir so fixtures (certs, logs) don't collide.
set_tests_properties(${rudesocket_tests} PROPERTIES TIMEOUT 60)
foreach(t ${rudesocket_tests})
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/run-${t}")
    set_tests_properties(${t} PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/run-${t}")
endforeach()
