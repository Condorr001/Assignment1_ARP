project("Assignment1_ARP_ConTo")

set(WRAP_FUNC_FILES
    wrapFuncs/wrapFunc.h
    wrapFuncs/wrapFunc.c)

set(UTILS_FILES
    utils/utils.h
    utils/utils.c)

add_library(wrapFunc ${WRAP_FUNC_FILES})
add_library(utils ${UTILS_FILES})

target_include_directories(
    wrapFunc
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../headers>
    )

target_include_directories(
    utils
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../headers>
    )

add_library(constants INTERFACE)
add_library(datastructs INTERFACE)

target_include_directories(
    constants
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../headers>
    )

target_include_directories(
    datastructs
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../headers>
    )
