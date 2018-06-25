cmake_minimum_required(VERSION 2.8.3)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(NVM_VERSION_MAJOR 0)
set(NVM_VERSION_MINOR 0)
set(NVM_VERSION_PATCH 1)
set(NVM_VERSION "${NVM_VERSION_MAJOR}.${NVM_VERSION_MINOR}.${NVM_VERSION_PATCH}")

project(liblightnvm C)
include(FeatureSummary)
include(use_c11)
include(CheckLibraryExists)
include(CheckFunctionExists)

#
# BACKENDS -- begin
#
set(NVM_BE_IOCTL_ENABLED ${UNIX} CACHE BOOL "be_ioctl: Linux IOCTL backend")
if (NVM_BE_IOCTL_ENABLED)
	add_definitions(-DNVM_BE_IOCTL_ENABLED)
endif()

set(NVM_BE_LBA_ENABLED ${UNIX} CACHE BOOL "be_lba: Linux IOCTL/LBA backend")
if (NVM_BE_LBA_ENABLED)
	add_definitions(-DNVM_BE_LBA_ENABLED)
endif()

set(NVM_BE_SYSFS_ENABLED ${UNIX} CACHE BOOL "be_sysfs: Linux IOCTL/sysfs backend")
if(NVM_BE_SYSFS_ENABLED)
	add_definitions(-DNVM_BE_SYSFS_ENABLED)
endif()

# SPDK is disabled by default
set(NVM_BE_SPDK_ENABLED FALSE CACHE BOOL "be_spdk: SPDK backend")
if(NVM_BE_SPDK_ENABLED)
	# TODO: Fix this hardcoded path SPDK headers
	include_directories(AFTER "/opt/spdk/include")
	add_definitions(-DNVM_BE_SPDK_ENABLED)
endif()
#
# BACKENDS -- end
#

if (WIN32)
	add_definitions(-D__USE_MINGW_ANSI_STDIO=1)
endif()

use_c11()
enable_c_flag("-std=c11")
enable_c_flag("-Wall")
enable_c_flag("-Wextra")
enable_c_flag("-pedantic")

if (NOT CMAKE_VERSION VERSION_LESS "2.8.12")
	find_package(OpenMP)
	if (OPENMP_FOUND)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	endif()
endif()

message( STATUS "CORE-CMAKE_C_FLAGS(${CMAKE_C_FLAGS})")

check_library_exists(c clock_gettime "" LIBC_HAS_CLOCK_GETTIME)
check_library_exists(rt clock_gettime "time.h" LIBRT_HAS_CLOCK_GETTIME)

# On Windows we assume it is available via the TDM-GCC compiler suite
if (WIN32)
	message( WARNING "Skipping check for clock_gettime..." )
else()
	if ((NOT LIBC_HAS_BLOCK_GETTIME) AND (NOT LIBRT_HAS_CLOCK_GETTIME))
		message( FATAL_ERROR "Cannot find clock_gettime" )
	endif()
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DNVM_DEBUG_ENABLED")

set(HEADER_FILES
	include/liblightnvm.h
	include/nvm_be.h
	include/nvm_debug.h
	include/nvm_timer.h
	include/nvm_dev.h
	include/nvm_omp.h
	include/liblightnvm_spec.h
	include/nvm_utils.h
	include/nvm_vblk.h
	include/nvm_nvme.h)

set(SOURCE_FILES
	src/nvm_spec.c
	src/nvm_be.c
	src/nvm_be_ioctl.c
	src/nvm_be_spdk.c
	src/nvm_be_sysfs.c
	src/nvm_be_lba.c
	src/nvm_dev.c
	src/nvm_buf.c
	src/nvm_bbt.c
	src/nvm_geo.c
	src/nvm_ver.c
	src/nvm_cmd.c
	src/nvm_addr.c
	src/nvm_vblk.c
	src/nvm_bounds.c
)

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/include/linux/uapi")

set(LNAME "lightnvm")

add_library(${LNAME} STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(${LNAME} PROPERTIES OUTPUT_NAME ${LNAME})

if(${NVM_BE_SPDK_ENABLED})
	link_directories(/opt/spdk/build/lib/)

	target_link_libraries(${LNAME} spdk_nvme)
	target_link_libraries(${LNAME} spdk_util)
	target_link_libraries(${LNAME} spdk_log)
	target_link_libraries(${LNAME} /opt/spdk/build/lib/libspdk_env_dpdk.a)
	target_link_libraries(${LNAME} /opt/spdk/dpdk/build/lib/librte_bus_pci.a)
	target_link_libraries(${LNAME} /opt/spdk/dpdk/build/lib/librte_eal.a)
	target_link_libraries(${LNAME} /opt/spdk/dpdk/build/lib/librte_mempool.a)
	target_link_libraries(${LNAME} /opt/spdk/dpdk/build/lib/librte_mempool_ring.a)
	target_link_libraries(${LNAME} /opt/spdk/dpdk/build/lib/librte_pci.a)
	target_link_libraries(${LNAME} /opt/spdk/dpdk/build/lib/librte_ring.a)
	target_link_libraries(${LNAME} rt)
	target_link_libraries(${LNAME} dl)
	target_link_libraries(${LNAME} numa)
	target_link_libraries(${LNAME} uuid)
endif()

install(TARGETS ${LNAME} DESTINATION lib COMPONENT lib)

install(FILES "${PROJECT_SOURCE_DIR}/include/liblightnvm_spec.h"
	DESTINATION include COMPONENT dev)
install(FILES "${PROJECT_SOURCE_DIR}/include/liblightnvm.h"
	DESTINATION include COMPONENT dev)

##
#
# Add the nvm_cli_* helpers to its own library and provide a public header
#
##
set(HEADER_FILES
	../include/liblightnvm_cli.h
)

set(CLI_LIB "lightnvm_cli")

set(SOURCE_FILES
	src/nvm_cli.c
)

add_library(${CLI_LIB} STATIC ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(${CLI_LIB} ${LNAME})
if ((NOT ${LIBC_HAS_CLOCK_GETTIME}) AND (${LIBRT_HAS_CLOCK_GETTIME}))
	target_link_libraries(${CLI_LIB} rt)
endif()
set_target_properties(${CLI_LIB} PROPERTIES OUTPUT_NAME ${CLI_LIB})
install(TARGETS ${CLI_LIB} DESTINATION lib COMPONENT cli)

# CLI
add_subdirectory(cli)

# TESTS
add_subdirectory(tests)

# Packages
#if ("${CMAKE_VERSION}" VERSION_GREATER "2.8.7")
#set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
#endif()
if (WIN32)
	set(CPACK_GENERATOR "NSIS" "ZIP")
else()
	set(CPACK_GENERATOR "DEB")
endif()
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_PACKAGE_VERSION "${NVM_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${NVM_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${NVM_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${NVM_VERSION_PATCH}")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}${NVM_VERSION_MAJOR}")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "LightNVM.io")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_COMPONENT_LIB_DESCRIPTION "liblightnvm: User space I/O library for Open-Channel SSDs")
set(CPACK_COMPONENT_DEV_DESCRIPTION "liblightnvm-dev: Public header and static library for liblightnvm")
set(CPACK_COMPONENT_CLI_DESCRIPTION "liblightnvm-cli: Command-line interface for liblightnvm")
set(CPACK_COMPONENT_TESTS_DESCRIPTION "liblightnvm-tests: Unit tests for liblightnvm")

include(CPack)
