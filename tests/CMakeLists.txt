cmake_minimum_required(VERSION 2.8)
set(TESTS true CACHE BOOL "Tests: Include test programs in build")
if (NOT TESTS)
	return()
endif()

find_library (CUNIT cunit)
if ( "${CUNIT}" STREQUAL "CUNIT-NOTFOUND" )
	message(FATAL_ERROR
		"Please install libcunit (e.g. 'libcunit1-dev' on Ubuntu 16.04)")
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DNVM_DEBUG_ENABLED")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

include_directories("${CMAKE_SOURCE_DIR}/include")
include_directories("${CMAKE_SOURCE_DIR}/include/linux/uapi")

set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/test_dev.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_vblock.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_vblock_rw.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_vblock_gp_n.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_beam.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_concur.c)

foreach(SRC_FN ${SOURCE_FILES})
	get_filename_component(SRC_FN_WE ${SRC_FN} NAME_WE)
	set(EXE_FN "nvm_${SRC_FN_WE}")
	add_executable(${EXE_FN} ${SRC_FN})
	target_link_libraries(${EXE_FN} lightnvm pthread udev cunit)
	install(TARGETS ${EXE_FN} DESTINATION bin)
endforeach()
