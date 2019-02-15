cmake_minimum_required(VERSION 2.8.3)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(NVM_VERSION_MAJOR 0 CACHE INTEGER "nvm_ver: NVM_VERSION_MAJOR")
set(NVM_VERSION_MINOR 1 CACHE INTEGER "nvm_ver: NVM_VERSION_MINOR")
set(NVM_VERSION_PATCH 7 CACHE INTEGER "nvm_ver: NVM_VERSION_PATH")
set(NVM_VERSION "${NVM_VERSION_MAJOR}.${NVM_VERSION_MINOR}.${NVM_VERSION_PATCH}")

project(liblightnvm C)
include(FeatureSummary)
include(use_c11)
include(CheckLibraryExists)
include(CheckFunctionExists)

# Add versioning
add_definitions(-DLNVM_VERSION_MAJOR=${NVM_VERSION_MAJOR})
add_definitions(-DLNVM_VERSION_MINOR=${NVM_VERSION_MINOR})
add_definitions(-DLNVM_VERSION_PATCH=${NVM_VERSION_PATCH})
add_definitions(-DLNVM_VERSION=${NVM_VERSION})

#
# BACKENDS -- begin
#
set(NVM_BE_IOCTL_ENABLED ${UNIX} CACHE BOOL "be_ioctl: Linux IOCTL backend")
if (NVM_BE_IOCTL_ENABLED)
	add_definitions(-DNVM_BE_IOCTL_ENABLED)
endif()

set(NVM_BE_LBD_ENABLED ${UNIX} CACHE BOOL "be_lbd: Linux IOCTL/LBD backend")
if (NVM_BE_LBD_ENABLED)
	add_definitions(-DNVM_BE_LBD_ENABLED)
	find_library(HAVE_LIBAIO NAMES aio)
	if(HAVE_LIBAIO)
		add_definitions(-DHAVE_LIBAIO)
	endif()
endif()

# SPDK is disabled by default
set(NVM_BE_SPDK_ENABLED FALSE CACHE BOOL "be_spdk: SPDK backend")
if(NVM_BE_SPDK_ENABLED)
	add_definitions(-DNVM_BE_SPDK_ENABLED)
endif()

# check if async is enabled
if(${NVM_BE_SPDK_ENABLED} OR (${NVM_BE_LBD_ENABLED} AND HAVE_LIBAIO))
	add_definitions(-DNVM_ASYNC_ENABLED)
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

set(NVM_OMP_ENABLED TRUE CACHE BOOL "nvm_omp: Compile with OpenMP?")
if(NVM_OMP_ENABLED)
	if (NOT CMAKE_VERSION VERSION_LESS "2.8.12")
		find_package(OpenMP)
		if (OPENMP_FOUND)
		set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
		endif()
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
	${PROJECT_SOURCE_DIR}/include/liblightnvm.h
	${PROJECT_SOURCE_DIR}/include/liblightnvm_util.h
	${PROJECT_SOURCE_DIR}/include/liblightnvm_spec.h
	${PROJECT_SOURCE_DIR}/include/nvm_async.h
	${PROJECT_SOURCE_DIR}/include/nvm_be.h
	${PROJECT_SOURCE_DIR}/include/nvm_dev.h
	${PROJECT_SOURCE_DIR}/include/nvm_omp.h
	${PROJECT_SOURCE_DIR}/include/nvm_sgl.h
	${PROJECT_SOURCE_DIR}/include/nvm_timer.h
	${PROJECT_SOURCE_DIR}/include/nvm_vblk.h)

set(SOURCE_FILES
	${PROJECT_SOURCE_DIR}/src/nvm_addr.c
	${PROJECT_SOURCE_DIR}/src/nvm_async.c
	${PROJECT_SOURCE_DIR}/src/nvm_bbt.c
	${PROJECT_SOURCE_DIR}/src/nvm_be.c
	${PROJECT_SOURCE_DIR}/src/nvm_be_ioctl.c
	${PROJECT_SOURCE_DIR}/src/nvm_be_lbd.c
	${PROJECT_SOURCE_DIR}/src/nvm_be_spdk.c
	${PROJECT_SOURCE_DIR}/src/nvm_be_nocd.c
	${PROJECT_SOURCE_DIR}/src/nvm_bounds.c
	${PROJECT_SOURCE_DIR}/src/nvm_bp.c
	${PROJECT_SOURCE_DIR}/src/nvm_buf.c
	${PROJECT_SOURCE_DIR}/src/nvm_cmd.c
	${PROJECT_SOURCE_DIR}/src/nvm_dev.c
	${PROJECT_SOURCE_DIR}/src/nvm_geo.c
	${PROJECT_SOURCE_DIR}/src/nvm_ret.c
	${PROJECT_SOURCE_DIR}/src/nvm_sgl.c
	${PROJECT_SOURCE_DIR}/src/nvm_spec.c
	${PROJECT_SOURCE_DIR}/src/nvm_vblk.c
	${PROJECT_SOURCE_DIR}/src/nvm_ver.c
)

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/include/linux/uapi")

set(LNAME "lightnvm")

add_library(${LNAME} STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(${LNAME} PROPERTIES OUTPUT_NAME ${LNAME})

if(${NVM_BE_SPDK_ENABLED})
	set(NVM_BE_SPDK_DEPS spdk_nvme spdk_util spdk_log spdk_env_dpdk rte_bus_pci rte_eal rte_mempool rte_mempool_ring rte_pci rte_ring rt dl numa uuid)
	foreach(NVM_BE_SPDK_DEP ${NVM_BE_SPDK_DEPS})
		target_link_libraries(${LNAME} ${NVM_BE_SPDK_DEP})
	endforeach()

	if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
		if(EXISTS "${ROOT}/usr/local/include/dpdk")
			include_directories("/usr/local/include/dpdk")
			add_definitions(-DNVM_BE_SPDK_CHOKE_PRINTING)
		endif()
	endif()

	try_compile(NVM_BE_SPDK_HAS_SGL
		"${CMAKE_BINARY_DIR}/temp"
		"${CMAKE_SOURCE_DIR}/cmake/try_compile_spdk_nvme_ctrlr_cmd_iov_raw_with_md.c"
		LINK_LIBRARIES ${NVM_BE_SPDK_DEPS}
		CMAKE_FLAGS ${CMAKE_C_FLAGS})

	if (NVM_BE_SPDK_HAS_SGL)
		add_definitions(-DNVM_BE_SPDK_HAS_SGL)
	endif()
endif()

if(${NVM_BE_LBD_ENABLED} AND HAVE_LIBAIO)
	target_link_libraries(${LNAME} aio)
endif()

install(TARGETS ${LNAME} DESTINATION lib COMPONENT lib)

install(FILES "${PROJECT_SOURCE_DIR}/include/liblightnvm_cli.h"
	DESTINATION include COMPONENT dev)
install(FILES "${PROJECT_SOURCE_DIR}/include/liblightnvm_spec.h"
	DESTINATION include COMPONENT dev)
install(FILES "${PROJECT_SOURCE_DIR}/include/liblightnvm_util.h"
	DESTINATION include COMPONENT dev)
install(FILES "${PROJECT_SOURCE_DIR}/include/liblightnvm.h"
	DESTINATION include COMPONENT dev)

##
#
# Add the nvm_cli_* helpers to its own library and provide a public header
#
##
set(HEADER_FILES
	${PROJECT_SOURCE_DIR}/include/liblightnvm_cli.h
)
set(SOURCE_FILES
	${PROJECT_SOURCE_DIR}/src/nvm_cli.c
)

set(CLI_LIB "lightnvm_cli")

include_directories("${PROJECT_SOURCE_DIR}/include")

add_library(${CLI_LIB} STATIC ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(${CLI_LIB} ${LNAME})
if ((NOT ${LIBC_HAS_CLOCK_GETTIME}) AND (${LIBRT_HAS_CLOCK_GETTIME}))
	target_link_libraries(${CLI_LIB} rt)
endif()
set_target_properties(${CLI_LIB} PROPERTIES LINKER_LANGUAGE C)
set_target_properties(${CLI_LIB} PROPERTIES OUTPUT_NAME ${CLI_LIB})
install(TARGETS ${CLI_LIB} DESTINATION lib COMPONENT cli)

# CLI
add_subdirectory(cli)

# TESTS
add_subdirectory(tests)

# EXAMPLES
add_subdirectory(examples)

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
set(CPACK_COMPONENT_EXAMPLES_DESCRIPTION "liblightnvm-examples: Unit tests for liblightnvm")

include(CPack)
