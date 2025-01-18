add_library(passthru INTERFACE)

target_include_directories(passthru INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(passthru INTERFACE
    pico_stdlib
    pico_multicore
    )

target_sources(passthru  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/passthru.c
    ${CMAKE_CURRENT_LIST_DIR}/passthru.h)