include(CheckCCompilerFlag)

function(use_c99)
	if (NOT CMAKE_VERSION VERSION_LESS "3.1")
		set(CMAKE_C_STANDARD 99 PARENT_SCOPE)
		return()
	endif()

	# This covers gcc-compatible compilers
	check_c_compiler_flag(-std=c99 HAS_GCC_C99)
	if (HAS_GCC_C99)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 " PARENT_SCOPE)
		return()
	endif()

	check_c_compiler_flag(-c99 HAS_PGI_C99)
	if (HAS_PGI_C99)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -c99 " PARENT_SCOPE)
		return()
	endif()

	# NOTE: Expand with further checks e.g. icc on Windows, and other
	# non-gcc compatible compilers.
endfunction(use_c99)
