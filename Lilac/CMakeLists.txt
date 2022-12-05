# CMakeList.txt : CMake project for Lilac, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.22)

project ("Lilac" VERSION 0.1)

include(FetchContent)
include(CheckSymbolExists)

set(CMAKE_REQUIRED_LIBRARIES "legacy_stdio_definitions.lib")

set(BUILD_SHARED_LIBS TRUE)

FetchContent_Declare(
	SFML3D
	GIT_REPOSITORY https://github.com/PierceLBrooks/SFML3D.git
	GIT_TAG 3dee
)

FetchContent_MakeAvailable(SFML3D)

# Add source to this project's executable.
include_directories("include")
include_directories("${sfml3d_SOURCE_DIR}/include")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  # set_property(TARGET Lilac PROPERTY CXX_STANDARD 20)
endif()

link_libraries(legacy_stdio_definitions)

add_executable (Lilac "src/Lilac.cpp")
target_link_libraries(Lilac sfml3d-graphics sfml3d-window sfml3d-system)

# TODO: Add tests and install targets if needed.
