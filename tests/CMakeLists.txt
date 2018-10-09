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
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

set(HEADER_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/include/nvm_test.h)

set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/test_dev.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_addr_conv.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_buf.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_vblk_wre.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_bbt.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_sgl.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_cmd_rprt.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_cmd_rprt_ordr.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_cmd_feat.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_cmd_erase.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_cmd_wre_scalar.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_cmd_wre_vector.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_cmd_copy.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_rules_read.c
	${CMAKE_CURRENT_SOURCE_DIR}/test_rules_write.c)

#
# static linking, against lightnvm_a, to avoid runtime dependency on liblightnvm
#
foreach(SRC_FN ${SOURCE_FILES})
	get_filename_component(SRC_FN_WE ${SRC_FN} NAME_WE)
	set(EXE_FN "nvm_${SRC_FN_WE}")
	add_executable(${EXE_FN} ${SRC_FN} util.c)
	target_link_libraries(${EXE_FN} ${CUNIT} pthread ${LNAME})
	install(TARGETS ${EXE_FN} DESTINATION bin COMPONENT tests)
endforeach()
