# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

##########################################################
# Generate the compile-time data.
# Each data item is saved as a CMake variable, that is passed
# as a preprocessor define to the compiler.
#
# If you add new data items, they must be added to timestamp.c,
# timestamp.h and the target_compile_definitions below.


# Set CMake variable BUILD_TIME to 'now'.
# This variable drives the CMake auto-update scripts, so
# this must not be commented out!
# (TIMESTAMP is a CMake built-in command)
string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S" UTC)
string(TIMESTAMP BUILD_DATE "%Y-%m-%d" UTC)

##########################################################
# Git versioning info.
# First a helper function, then use it for select git data.
find_package(Git)
function(git_cmd optstring output)
	# convert space separated string to CMake list, so it gets
	# split into argvs for the git command (and not passed, spaces and all,
	# in argv[1])
	string(REPLACE " " ";" cmd_list ${optstring})
	execute_process(
	COMMAND
		${GIT_EXECUTABLE} ${cmd_list}
	WORKING_DIRECTORY
		${CMAKE_SOURCE_DIR}
	OUTPUT_VARIABLE
		res
	OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	set( ${output} ${res} PARENT_SCOPE )
endfunction()

git_cmd( "show -s --format=%H" GIT_HASH)
git_cmd( "show -s --format=%h" GIT_SHORT_HASH)
git_cmd( "diff -w --shortstat" GIT_DIRTY)
if( ${GIT_VERSION_STRING} VERSION_GREATER_EQUAL 2.22 )
	git_cmd( "branch --show-current" GIT_BRANCH)
endif()


# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(ninoble C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(passthru)

# Add executable. Default name is the project name, version 0.1
# Standalone example that reads from the on board temperature sensor and sends notifications via BLE
# Flashes slowly each second to show it's running
add_executable(sniffer
    main_sniffer.c kiss.c ax25.c oled.c
    )

target_link_libraries(sniffer PUBLIC
    pico_stdlib
    pico_cyw43_arch_none
    hardware_i2c)

target_include_directories(sniffer PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For btstack config
)

pico_add_extra_outputs(sniffer)

pico_enable_stdio_uart(sniffer 0)
pico_enable_stdio_usb(sniffer 1)


# Main BLE program

add_executable(ninoble
    main_aprs.c bt.c kiss.c pconfig.c shell.c ax25.c oled.c aprs.c sd_config.c logger.c
    )

# Add SD card library
add_subdirectory(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src ninoble)

target_link_libraries(ninoble PUBLIC
    pico_stdlib
    pico_btstack_ble
    pico_btstack_classic
    pico_btstack_cyw43
    pico_multicore
    pico_cyw43_arch_none
    hardware_adc
    hardware_i2c
    hardware_spi
    passthru
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
    )

target_compile_definitions(ninoble
	PRIVATE -DBUILD_TIME="${BUILD_TIME}"
    PRIVATE -DBUILD_DATE="${BUILD_DATE}"
	PRIVATE -DGIT_HASH="${GIT_HASH}"
	PRIVATE -DGIT_SHORT_HASH="${GIT_SHORT_HASH}"
	PRIVATE -DGIT_BRANCH="${GIT_BRANCH}"
	PRIVATE -DGIT_DIRTY="${GIT_DIRTY}"
	PRIVATE -DCPACK_VERSION="${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}"
)

target_include_directories(ninoble 
  PRIVATE ${CMAKE_CURRENT_LIST_DIR} # For btstack config
)

pico_btstack_make_gatt_header(ninoble 
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/ninoble_gatt_services.gatt")

pico_add_extra_outputs(ninoble)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(ninoble 0)
pico_enable_stdio_usb(ninoble 1)