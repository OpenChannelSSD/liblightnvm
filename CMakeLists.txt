cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(FeatureSummary)
project(liblightnvm C)

find_package(Threads REQUIRED)
find_package(udev REQUIRED liblightnvm)
set_package_properties(udev PROPERTIES DESCRIPTION "Queriying OpenChannel SSDs via sysfs/udev.")
set_package_properties(udev PROPERTIES TYPE REQUIRED PURPOSE "Queriying OpenChannel SSDs via sysfs/udev.")
if ( NOT ${UDEV_FOUND} )
	message(FATAL_ERROR
		"Please install libudev ('libudev-dev' on Ubuntu)")
endif()

set(NVM_VERSION_MAJOR 1)
set(NVM_VERSION_MINOR 0)
set(NVM_VERSION_PATCH 0)
set(NVM_VERSION ${NVM_VERSION_MAJOR}.${NVM_VERSION_MINOR}.${NVM_VERSION_PATCH})

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DNVM_DEBUG_ENABLED")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

set(HEADER_FILES
	include/liblightnvm.h
	include/likely.h
	include/dflash_ioctl.h
	include/nvm.h
	include/nvm_atomic.h
	include/nvm_beam.h
	include/nvm_debug.h
	include/nvm_utils.h
	include/uthash.h)

set(SOURCE_FILES
	src/nvm_beam.c
	src/nvm_dev.c
	src/nvm_mgmt.c
	src/nvm_tgt.c
	src/nvm_utils.c
	src/nvm_vblock.c)

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/include/linux/uapi")

add_library(lightnvm SHARED ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(lightnvm PROPERTIES
	VERSION ${NVM_VERSION}
	SOVERSION ${NVM_VERSION_MAJOR}
	)
target_link_libraries(lightnvm udev ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS lightnvm DESTINATION lib)
install(FILES "${PROJECT_SOURCE_DIR}/include/liblightnvm.h" DESTINATION include)

add_subdirectory(examples)
add_subdirectory(tests)

# For building debian packages
set(CPACK_GENERATOR "DEB" "TGZ")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Javier González")
include(CPack)
