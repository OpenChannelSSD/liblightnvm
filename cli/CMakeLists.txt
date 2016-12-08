cmake_minimum_required(VERSION 2.8)
set(CLI false CACHE BOOL "Examples: Include example programs in build")
if (NOT CLI)
	return()
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DNVM_DEBUG_ENABLED")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

include_directories("${CMAKE_SOURCE}/include")

set(HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/nvm_cli.h)

set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/dev.c
	${CMAKE_CURRENT_SOURCE_DIR}/bbt.c
	${CMAKE_CURRENT_SOURCE_DIR}/addr.c
	${CMAKE_CURRENT_SOURCE_DIR}/vblk.c
	${CMAKE_CURRENT_SOURCE_DIR}/sblk.c)

#
# We link against the lightnvm_a to avoid the runtime dependency on liblightnvm
# NOTE: The dependency on libudev is carried on from liblightnvm
#
foreach(SRC_FN ${SOURCE_FILES})
	get_filename_component(SRC_FN_WE ${SRC_FN} NAME_WE)
	set(EXE_FN "nvm_${SRC_FN_WE}")
    add_executable(${EXE_FN} ${SRC_FN} ${CMAKE_CURRENT_SOURCE_DIR}/nvm_cli.c)
    target_include_directories(${EXE_FN} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
	target_link_libraries(${EXE_FN} lightnvm_a)

	install(TARGETS ${EXE_FN} DESTINATION bin)
endforeach()

