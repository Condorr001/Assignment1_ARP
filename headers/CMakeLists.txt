project("Assignment1_ARP_ConTo")

set(WRAP_FUNC_FILES
    wrapFuncs/wrapFunc.h
    wrapFuncs/wrapFunc.c)

add_library(wrapFunc ${WRAP_FUNC_FILES})

target_include_directories(
    wrapFunc
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../headers>
    )

add_library(constants INTERFACE)

target_include_directories(
    constants
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../headers>
    )
