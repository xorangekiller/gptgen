cmake_minimum_required(VERSION 3.15)
project(gptgen VERSION 1.2.1 LANGUAGES CXX)

option(BUILD_STATIC "Build a fully static executable" OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
	set(USING_CLANG TRUE)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
	set(USING_GCC TRUE)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	set(USING_MSVC TRUE)
endif()

if(USING_CLANG OR USING_GCC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wextra -Werror -O0 -ggdb -DDEBUG")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "${CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT} -O2 -ggdb")

	if(WIN32)
		add_compile_definitions(WINDOWS_BUILD)
	endif()
elseif(USING_MSVC)
	add_compile_definitions(WINDOWS_BUILD)
elseif(APPLE)
	add_compile_definitions(MACOS_BUILD)
endif()

if(BUILD_STATIC)
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		message(STATUS "Enabling static build")
	else()
		message(STATUS "Enabling static build (recommend setting -DCMAKE_BUILD_TYPE=Release)")
	endif()

	if(MSVC)
		# Link the MSVC Runtime Library statically (/MT).
		# See https://docs.microsoft.com/en-us/cpp/build/reference/md-mt-ld-use-run-time-library
		# and https://cmake.org/cmake/help/latest/variable/CMAKE_MSVC_RUNTIME_LIBRARY.html
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	else()
		# GCC and Clang support the "-static" flag to enable fully static linking.
		# See https://gcc.gnu.org/onlinedocs/gcc-15.2.0/gcc/Link-Options.html#index-static
		# and https://cmake.org/cmake/help/latest/command/add_link_options.html
		add_link_options("-static")
	endif()
endif()

add_executable(gptgen "gptgen.cpp")

if(WIN32)
	install(TARGETS gptgen DESTINATION gptgen)
else()
	install(TARGETS gptgen DESTINATION sbin)
endif()
