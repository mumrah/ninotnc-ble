# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(ninoble_tests C CXX ASM)

add_compile_options(-Wall -Werror -Wextra -pedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-gnu-binary-literal -Wno-implicit-function-declaration -Wno-incompatible-function-pointer-types)

add_executable(kiss_test ../kiss.c ../kiss.h test_kiss.c munit.c)
target_include_directories(kiss_test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
)

add_executable(shell_test ../shell.c ../shell.h test_shell.c munit.c)
target_include_directories(shell_test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
)

add_executable(ax25_test ../aprs.c ../aprs.h ../ax25.c ../ax25.h ../kiss.c ../kiss.h test_ax25.c munit.c)
target_include_directories(ax25_test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
)

add_executable(passthru_test ../passthru/passthru.c ../passthru/passthru.h pico/mutex.h test_passthru.c munit.c)
target_include_directories(passthru_test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
)


enable_testing()
add_test(NAME kiss_test COMMAND kiss_test)
add_test(NAME shell_test COMMAND shell_test)
add_test(NAME ax25_test COMMAND ax25_test)
add_test(NAME passthru_test COMMAND passthru_test)
