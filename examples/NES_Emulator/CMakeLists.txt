cmake_minimum_required(VERSION 3.16)
project(nes_emulator CXX C)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

include(FetchContent)

# --------------------------------------------------------------------------
# LaiNES — NES core (GPL-3.0; source fetched at build time, not vendored).
#
# Upstream: https://github.com/AndreaOrru/LaiNES
# No tagged releases exist, so we pin the HEAD of the default branch at the
# time this file was written. Update the hash to re-pin.
# --------------------------------------------------------------------------
set(LAINES_GIT_TAG "1f3d126171104ec57e515641ea3b5291d383ebdf"
    CACHE STRING "LaiNES commit to fetch (master @ 2025-11-06)")

FetchContent_Declare(
    laines
    GIT_REPOSITORY https://github.com/AndreaOrru/LaiNES.git
    GIT_TAG        ${LAINES_GIT_TAG}
    GIT_SHALLOW    FALSE
    GIT_SUBMODULES_RECURSE TRUE
)

FetchContent_GetProperties(laines)
if(NOT laines_POPULATED)
    FetchContent_Populate(laines)
endif()

# LaiNES ships an SConstruct only. Build its sources into a static library
# here, excluding the upstream SDL frontend (src/main.cpp + src/gui.cpp +
# src/menu.cpp + src/config.cpp) so we can substitute our own 320x170 UI.
# Also include the Nes_Snd_Emu sources under lib/*.cpp (the APU backend).
file(GLOB LAINES_SRC_CPP CONFIGURE_DEPENDS
    "${laines_SOURCE_DIR}/src/*.cpp"
    "${laines_SOURCE_DIR}/src/mappers/*.cpp")
file(GLOB LAINES_LIB_CPP CONFIGURE_DEPENDS
    "${laines_SOURCE_DIR}/lib/*.cpp")
set(LAINES_SOURCES ${LAINES_SRC_CPP} ${LAINES_LIB_CPP})

list(FILTER LAINES_SOURCES EXCLUDE REGEX ".*/src/main\\.cpp$")
list(FILTER LAINES_SOURCES EXCLUDE REGEX ".*/src/gui\\.cpp$")
list(FILTER LAINES_SOURCES EXCLUDE REGEX ".*/src/menu\\.cpp$")
list(FILTER LAINES_SOURCES EXCLUDE REGEX ".*/src/config\\.cpp$")

add_library(laines_core STATIC ${LAINES_SOURCES})

target_include_directories(laines_core PUBLIC
    "${laines_SOURCE_DIR}/src/include"
    "${laines_SOURCE_DIR}/lib/include"
    "${laines_SOURCE_DIR}/simpleini")

target_compile_options(laines_core PRIVATE -w -fpermissive)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE IMPORTED_TARGET SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

target_include_directories(laines_core PUBLIC ${SDL2_INCLUDE_DIRS})
target_link_libraries(laines_core PUBLIC ${SDL2_LIBRARIES})

# --------------------------------------------------------------------------
# Our frontend
# --------------------------------------------------------------------------
add_executable(nes-emulator
    src/main.cpp
    src/viewport.cpp
    src/input.cpp)

target_include_directories(nes-emulator PRIVATE
    src
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS})

target_link_libraries(nes-emulator PRIVATE
    laines_core
    ${SDL2_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    pthread)

target_compile_options(nes-emulator PRIVATE -Wall -Wextra -O2)
