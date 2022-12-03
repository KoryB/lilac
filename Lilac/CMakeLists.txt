# CMakeList.txt : CMake project for Lilac, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.22)

project ("Lilac" VERSION 0.1)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF)

FetchContent_Declare(
	SFML
	GIT_REPOSITORY https://github.com/SFML/SFML.git
	GIT_TAG 2.5.1
)

FetchContent_MakeAvailable(SFML)

# Add source to this project's executable.
include_directories("include")
include_directories("lib/SpatiumLib/include")
add_executable (Lilac "src/Lilac.cpp" "include/Lilac.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Lilac PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(Lilac sfml-graphics sfml-window sfml-system)

# TODO: Add tests and install targets if needed.
