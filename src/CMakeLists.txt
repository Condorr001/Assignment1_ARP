project("Assignment1_ARP_ConTo")

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(master master.c)
add_executable(server server.c)
add_executable(drone drone.c)
add_executable(map map.c)

target_link_libraries(master wrapFunc constants)
target_link_libraries(server wrapFunc constants)
target_link_libraries(drone wrapFunc constants)
target_link_libraries(map wrapFunc constants ${CURSES_LIBRARIES})
