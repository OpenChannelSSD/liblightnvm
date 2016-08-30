cmake_minimum_required(VERSION 2.8)
project(liblightnvm_examples C)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DNVM_DEBUG_ENABLED")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

include_directories("${PROJECT_SOURCE_DIR}/../include")

set(SOURCE_FILES
	${PROJECT_SOURCE_DIR}/info.c
	${PROJECT_SOURCE_DIR}/vblock_pio_1.c
	${PROJECT_SOURCE_DIR}/vblock_pio_n.c)

foreach(SRC_FN ${SOURCE_FILES})
	get_filename_component(SRC_FN_WE ${SRC_FN} NAME_WE)
	set(EXE_FN "nvm_ex_${SRC_FN_WE}")
	add_executable(${EXE_FN} ${SRC_FN})
	target_link_libraries(${EXE_FN} lightnvm pthread udev)
	install(TARGETS ${EXE_FN}  DESTINATION bin)
endforeach()

# This one is special... using different flags and libraries
#set(SOURCE_FILES ${PROJECT_SOURCE_DIR}/nvmfs.c)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")
#add_executable(nvmfs ${PROJECT_SOURCE_DIR}/nvmfs.c)
#target_link_libraries(nvmfs lightnvm pthread udev fuse)
#install(TARGETS nvmfs DESTINATION bin)
