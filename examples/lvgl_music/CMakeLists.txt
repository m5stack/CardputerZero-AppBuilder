cmake_minimum_required(VERSION 3.16)
project(lvgl_music C)

get_filename_component(_cz_root "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH "${_cz_root}/sdk/cmake")
include(CZApp)

cz_add_lvgl_app(lvgl_music SOURCES src/main.c)
