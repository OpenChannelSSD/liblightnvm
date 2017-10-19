cmake_minimum_required(VERSION 2.8)
set(TESTS true CACHE BOOL "Tests: Include test programs in build")
if (NOT TESTS)
	return()
endif()

find_library (CUNIT NAMES cunit)
if ( "${CUNIT}" STREQUAL "CUNIT-NOTFOUND" )
	message(FATAL_ERROR
		"Please install libcunit (e.g. 'libcunit1-dev' on Ubuntu 16.04)")
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DNVM_DEBUG_ENABLED")

message("TESTS-CMAKE_C_FLAGS(${CMAKE_C_FLAGS})")

include_directories("${CMAKE_SOURCE_DIR}/include")
include_directories("${CMAKE_SOURCE_DIR}/include/linux/uapi")

set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/test_dev.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_io_s20.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_addr_io.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_addr_rio.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_addr_conv.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_vblk.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_bbt.c)

#
# static linking, against lightnvm_a, to avoid runtime dependency on liblightnvm
#
foreach(SRC_FN ${SOURCE_FILES})
	get_filename_component(SRC_FN_WE ${SRC_FN} NAME_WE)
	set(EXE_FN "nvm_${SRC_FN_WE}")
	add_executable(${EXE_FN} ${SRC_FN})
	target_link_libraries(${EXE_FN} ${CUNIT} pthread lightnvm_a)
	install(TARGETS ${EXE_FN} DESTINATION bin COMPONENT tests)
endforeach()
