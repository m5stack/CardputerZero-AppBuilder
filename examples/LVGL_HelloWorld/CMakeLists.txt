cmake_minimum_required(VERSION 3.16)
project(lvgl_hello C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)

# Disable LVGL's Helium SIMD assembly path BEFORE FetchContent_MakeAvailable —
# its .S sources fail to assemble on ubuntu-24.04-arm's default binutils.
# LVGL's CMakeLists reads these cache variables during configure.
set(LV_USE_OS        "NONE" CACHE STRING "" FORCE)
set(LV_USE_DRAW_SW_ASM 0     CACHE BOOL   "" FORCE)
set(LV_USE_NEMA_GFX  0       CACHE BOOL   "" FORCE)

# Fetch LVGL v9.2.2. Do not vendor the source tree.
FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v9.2.2
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(lvgl)

# LVGL v9.2.2's CMakeLists unconditionally globs src/**/*.S, pulling in
# lv_blend_helium.S which fails to assemble on ubuntu-24.04-arm (plain
# binutils, no -mfpu=auto). Strip .S from the target's sources post-hoc.
get_target_property(_lvgl_src lvgl SOURCES)
if(_lvgl_src)
    list(FILTER _lvgl_src EXCLUDE REGEX "\\.S$")
    set_target_properties(lvgl PROPERTIES SOURCES "${_lvgl_src}")
endif()

# Tell LVGL to use our lv_conf.h via simple include.
target_compile_definitions(lvgl PUBLIC
    LV_CONF_INCLUDE_SIMPLE=1
    LV_USE_LINUX_FBDEV=1
    LV_USE_EVDEV=1
)

# Make our lv_conf.h visible to LVGL and to this project.
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(hello_lvgl src/main.c)

target_include_directories(hello_lvgl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${lvgl_SOURCE_DIR}
)

# LVGL v9.2 ships a separate linux backend target when built with the above flags.
# Link it if present, otherwise link the core lvgl target only.
set(_lvgl_link_libs lvgl)

if(TARGET lvgl_linux)
    list(APPEND _lvgl_link_libs lvgl_linux)
endif()

if(TARGET lvgl::lvgl_drivers)
    list(APPEND _lvgl_link_libs lvgl::lvgl_drivers)
endif()

target_link_libraries(hello_lvgl PRIVATE ${_lvgl_link_libs} m pthread)
