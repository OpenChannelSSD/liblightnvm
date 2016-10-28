cmake_minimum_required(VERSION 2.8.3)
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

set(NVM_VERSION_MAJOR 0)
set(NVM_VERSION_MINOR 0)
set(NVM_VERSION_PATCH 1)
set(NVM_VERSION "${NVM_VERSION_MAJOR}.${NVM_VERSION_MINOR}.${NVM_VERSION_PATCH}")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DNVM_DEBUG_ENABLED")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

set(HEADER_FILES
	include/liblightnvm.h
	include/likely.h
	include/nvm.h
	include/nvm_atomic.h
	include/nvm_beam.h
	include/nvm_debug.h
	include/nvm_util.h
	include/uthash.h)

set(SOURCE_FILES
	src/nvm_beam.c
	src/nvm_dev.c
	src/nvm_util.c
	src/nvm_sblk.c
	src/nvm_vblk.c
	src/nvm_addr.c)

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/include/linux/uapi")

#
# NOTE: We build lightnvm as both a SHARED and a STATIC library
#
# We compile it twice since the nice cmake feature of "objlib" is not supported
# until cmake 2.8.8 and we would like to compile out of the box with cmake 2.8.7
# which ships with Ubuntu 12.04 and other old distributions
#
# The good part is that the static lib with not use the -fPIC flag, which it
# would otherwise do with re-using the object-files for both a shared and a
# static library
#

#
# SHARED: Here we define the shared library with versioning and all that jazz
#
add_library(lightnvm SHARED ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(lightnvm PROPERTIES
	VERSION ${NVM_VERSION}
	SOVERSION ${NVM_VERSION_MAJOR}
	)
target_link_libraries(lightnvm udev ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS lightnvm DESTINATION lib)
install(FILES "${PROJECT_SOURCE_DIR}/include/liblightnvm.h"
	DESTINATION include)

#
# STATIC: Here we define the static library
#
add_library(lightnvm_a STATIC ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(lightnvm_a udev ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(lightnvm_a PROPERTIES OUTPUT_NAME lightnvm)

install(TARGETS lightnvm_a DESTINATION lib)

# EXAMPLES
add_subdirectory(examples)

# TESTS
add_subdirectory(tests)

# Packages
set(CPACK_GENERATOR "DEB" "TGZ")
set(CPACK_PACKAGE_VERSION "${NVM_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${NVM_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${NVM_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${NVM_VERSION_PATCH}")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "liblightnvm: Userspace I/O library for LightNVM")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Javier González")
if ("${CMAKE_VERSION}" VERSION_GREATER "2.8.7")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()

include(CPack)
