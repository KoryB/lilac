# CMakeList.txt : CMake project for Lilac, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.22)

project ("Lilac" VERSION 0.0.1)

include(FetchContent)

set(BUILD_SHARED_LIBS TRUE) 

FetchContent_Declare(
	SFML3D
	GIT_REPOSITORY https://github.com/PierceLBrooks/SFML3D.git
	GIT_TAG vs2015
)

FetchContent_MakeAvailable(SFML3D)

# Add source to this project's executable.
include_directories("include")
include_directories("lib/GLEW/include")
include_directories("${sfml3d_SOURCE_DIR}/include")

# Wouldn't be a bad idea to setup different for Win32 vs x64
link_directories("lib/GLEW/lib/Release/x64")

add_executable (Lilac "src/Lilac/Lilac.cpp")
add_executable (LilacRaytracer "src/Lilac/LilacRaytracer.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Lilac PROPERTY CXX_STANDARD 20)
  set_property(TARGET LilacRaytracer PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(Lilac sfml3d-graphics sfml3d-window sfml3d-system)
target_link_libraries(LilacRaytracer sfml3d-graphics sfml3d-window sfml3d-system glew32)

# TODO: Add tests and install targets if needed.

add_custom_command(TARGET Lilac POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E copy_directory
		"${sfml3d_BINARY_DIR}/lib"
		$<TARGET_FILE_DIR:Lilac>)

add_custom_command(TARGET LilacRaytracer POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E copy_directory
		"${sfml3d_BINARY_DIR}/lib"
		$<TARGET_FILE_DIR:LilacRaytracer>)

add_custom_command(TARGET LilacRaytracer POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different
		"${PROJECT_SOURCE_DIR}/lib/GLEW/bin/Release/x64/glew32.dll"
		$<TARGET_FILE_DIR:LilacRaytracer>)
