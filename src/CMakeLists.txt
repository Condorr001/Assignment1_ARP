project("Assignment1_ARP_ConTo")


add_executable(master master.c)
add_executable(server server.c)
add_executable(drone drone.c)

target_link_libraries(master wrapFunc constants)
target_link_libraries(server wrapFunc constants)
target_link_libraries(drone wrapFunc constants)
