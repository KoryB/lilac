# CMakeList.txt : CMake project for Lilac, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.22)

project ("Lilac" VERSION 0.0.1)

include(FetchContent)

set(BUILD_SHARED_LIBS TRUE) 
set(CMAKE_BUILD_TYPE Debug)

FetchContent_Declare(
	SFML3D
	GIT_REPOSITORY https://github.com/PierceLBrooks/SFML3D.git
	GIT_TAG vs2015
)

FetchContent_Declare(
	GLM
	GIT_REPOSITORY https://github.com/g-truc/glm.git
	GIT_TAG 0.9.9.8
)

FetchContent_MakeAvailable(SFML3D)
FetchContent_MakeAvailable(GLM)

# Add source to this project's executable.
include_directories("include")
include_directories("lib/GLEW/include")
include_directories("${sfml3d_SOURCE_DIR}/include")
include_directories("${glm_SOURCE_DIR}/")

# Wouldn't be a bad idea to setup different for Win32 vs x64
link_directories("lib/GLEW/lib/Release/x64")

add_executable(LilacRaytracer "src/Lilac/LilacRaytracer.cpp" "src/Lilac/Shader.cpp" "src/Lilac/Program.cpp" "src/Lilac/File.cpp" "include/strutil/strutil.h" "include/Lilac/SparseVoxelOctree.h" "src/Lilac/SparseVoxelOctree.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET LilacRaytracer PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(LilacRaytracer sfml3d-graphics sfml3d-window sfml3d-system glew32)

# TODO: Add tests and install targets if needed.


# File copying, for some reason it really doesn't like to copy multiple times, perhaps because files exist?
add_custom_command(TARGET LilacRaytracer POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E copy_directory
		"${sfml3d_BINARY_DIR}/lib"
		$<TARGET_FILE_DIR:LilacRaytracer>)

add_custom_command(TARGET LilacRaytracer POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E copy_directory
		"${PROJECT_SOURCE_DIR}/resources"
		"$<TARGET_FILE_DIR:LilacRaytracer>/resources")

add_custom_command(TARGET LilacRaytracer POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different
		"${PROJECT_SOURCE_DIR}/lib/GLEW/bin/Release/x64/glew32.dll"
		$<TARGET_FILE_DIR:LilacRaytracer>)
