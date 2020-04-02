cmake_minimum_required(VERSION 3.16)
project(astera VERSION 0.1.0 LANGUAGES C)

#list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0075 NEW)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_FIND_FRAMEWORK FIRST)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LNIK ccache)
endif(CCACHE_FOUND)

find_package(OpenGL REQUIRED)

# TODO: put set(OPENAL_<option> values here)
set(ALSOFT_INSTALL OFF)
add_subdirectory(${PROJECT_SOURCE_DIR}/dep/openal-soft)

set(ALSOFT_DLOPEN ON)
set(ALSOFT_WERROR OFF)
set(ALSOFT_UTILS OFF)
set(ALSOFT_EXAMPLES OFF)
set(ALSOFT_TESTS OFF)
set(ALSOFT_CONFIG OFF)
set(ALSOFT_HRTF_DEFS OFF)
set(ALSOFT_INSTALL OFF)

# set(GLFW_<option>)
set(BUILD_SHARED_LIBS ON)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
set(GLFW_VULKAN_STATIC OFF)

add_subdirectory(${PROJECT_SOURCE_DIR}/dep/glfw)

if (NOT TARGET OpenAL::AL)
  add_library(OpenAL::AL ALIAS OpenAL)
endif()

file(GLOB sources CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.c)
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${sources})
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/dep/nanovg/nanovg.c)
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/dep/misc/getopt.c)
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/dep/misc/zip.c)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/dep/openal-soft/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/dep/glad/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/dep/misc>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/dep>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INCLUDE_INSTALLDIR}>)
target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    $<$<PLATFORM_ID:FreeBSD>:FreeBSD>
    $<$<PLATFORM_ID:OpenBSD>:OpenBSD>
    $<$<PLATFORM_ID:NetBSD>:NetBSD>
    $<$<PLATFORM_ID:Darwin>:OSX>)
target_compile_features(${PROJECT_NAME} PRIVATE c_std_99)
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    OpenGL::GL
    OpenAL::AL
    m
    glfw)
