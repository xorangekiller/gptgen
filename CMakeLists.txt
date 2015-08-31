cmake_minimum_required(VERSION 3.0)
project(gptgen VERSION 1.1 LANGUAGES CXX)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
	set(USING_CLANG TRUE)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
	set(USING_GCC TRUE)
endif()

if(USING_CLANG OR USING_GCC)
	set(CMAKE_CXX_FLAGS "-Wall")
	set(CMAKE_CXX_FLAGS_DEBUG "-Wextra -Werror -O0 -ggdb -DDEBUG")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "-O2 -ggdb")

	if(WIN32)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWE_ARE_WINDOWS")
	endif()
endif()

add_executable(gptgen "gptgen.cpp")
