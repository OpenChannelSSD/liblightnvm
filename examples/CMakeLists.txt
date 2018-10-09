cmake_minimum_required(VERSION 2.8)
set(EXAMPLES true CACHE BOOL "Examples: Include example programs in build")
if (NOT EXAMPLES)
	return()
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DNVM_DEBUG_ENABLED")

include_directories("${PROJECT_SOURCE_DIR}/include")
message("EXAMPLES-CMAKE_C_FLAGS(${CMAKE_C_FLAGS})")

set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/async-ex01-completion-wait.c
	${CMAKE_CURRENT_SOURCE_DIR}/async-ex02-completion-poke.c
	${CMAKE_CURRENT_SOURCE_DIR}/async-ex03-small-queue.c
	${CMAKE_CURRENT_SOURCE_DIR}/async-ex12-horz.c
	${CMAKE_CURRENT_SOURCE_DIR}/sync-ex01-ewr.c
	${CMAKE_CURRENT_SOURCE_DIR}/vblk-ex01-ewr.c)

#
# static linking, against lightnvm_a, to avoid runtime dependency on liblightnvm
#
foreach(SRC_FN ${SOURCE_FILES})
	get_filename_component(SRC_FN_WE ${SRC_FN} NAME_WE)

	set(EXE_FN "nvm_examples_${SRC_FN_WE}")
	add_executable(${EXE_FN} ${SRC_FN})
	target_link_libraries(${EXE_FN} ${LNAME} ${CLI_LIB})
	target_include_directories(${EXE_FN} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

	install(TARGETS ${EXE_FN} DESTINATION bin COMPONENT examples)
endforeach()

