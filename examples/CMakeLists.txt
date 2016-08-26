cmake_minimum_required(VERSION 2.8)
project(liblightnvm_examples C)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DNVM_DEBUG_ENABLED")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

include_directories("${PROJECT_SOURCE_DIR}/../include")

link_directories("/tmp/nvm_install/lib")

add_executable(nvm_ex_devinfo
	${PROJECT_SOURCE_DIR}/devinfo.c)
target_link_libraries(nvm_ex_devinfo lightnvm pthread udev)
install(TARGETS nvm_ex_devinfo DESTINATION bin)

add_executable(nvm_ex_tgtinfo
	${PROJECT_SOURCE_DIR}/tgtinfo.c)
target_link_libraries(nvm_ex_tgtinfo lightnvm pthread udev)
install(TARGETS nvm_ex_tgtinfo DESTINATION bin)

add_executable(nvm_ex_vblock
	${PROJECT_SOURCE_DIR}/vblock.c)
target_link_libraries(nvm_ex_vblock lightnvm pthread udev)
install(TARGETS nvm_ex_vblock DESTINATION bin)

add_executable(nvm_ex_vblock_rw
	${PROJECT_SOURCE_DIR}/vblock_rw.c)
target_link_libraries(nvm_ex_vblock_rw lightnvm pthread udev)
install(TARGETS nvm_ex_vblock_rw DESTINATION bin)

add_executable(nvm_ex_vblock_wr
	${PROJECT_SOURCE_DIR}/vblock_wr.c)
target_link_libraries(nvm_ex_vblock_wr lightnvm pthread udev)
install(TARGETS nvm_ex_vblock_wr DESTINATION bin)

add_executable(nvm_ex_vblock_rw_multilun
	${PROJECT_SOURCE_DIR}/vblock_rw_multilun.c)
target_link_libraries(nvm_ex_vblock_rw_multilun lightnvm pthread udev)
install(TARGETS nvm_ex_vblock_rw_multilun DESTINATION bin)

add_executable(nvm_ex_vblock_rw_all
	${PROJECT_SOURCE_DIR}/vblock_rw_all.c)
target_link_libraries(nvm_ex_vblock_rw_all lightnvm pthread udev)
install(TARGETS nvm_ex_vblock_rw_all DESTINATION bin)

add_executable(nvm_ex_vblock_reserve_n
	${PROJECT_SOURCE_DIR}/vblock_reserve_n.c)
target_link_libraries(nvm_ex_vblock_reserve_n lightnvm pthread udev)
install(TARGETS nvm_ex_vblock_reserve_n DESTINATION bin)

