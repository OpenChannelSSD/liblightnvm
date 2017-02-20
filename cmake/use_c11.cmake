include(CheckCCompilerFlag)

function(use_c11)
	if (NOT CMAKE_VERSION VERSION_LESS "3.1")
		set(CMAKE_C_STANDARD 11 PARENT_SCOPE)
		return()
	endif()

	# This covers gcc-compatible compilers
	check_c_compiler_flag(-std=c11 HAS_GCC_C11)
	if (HAS_GCC_C11)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 " PARENT_SCOPE)
		return()
	endif()

	check_c_compiler_flag(-c11 HAS_PGI_C11)
	if (HAS_PGI_C11)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -c11 " PARENT_SCOPE)
		return()
	endif()

	# NOTE: Expand with further checks e.g. icc on Windows, and other
	# non-gcc compatible compilers.
endfunction(use_c11)

function(enable_c_flag flag)
	string(FIND "${CMAKE_C_FLAGS}" "${flag}" flag_already_set)
	if(flag_already_set EQUAL -1)
		check_c_compiler_flag("${flag}" flag_supported)
		if(flag_supported)
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" PARENT_SCOPE)
		endif()
	endif()
endfunction()
