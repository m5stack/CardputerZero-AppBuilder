cmake_minimum_required(VERSION 3.16)
project(hello_cz C)

# Locate the AppBuilder SDK (two levels up from this example).
get_filename_component(_cz_root "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH "${_cz_root}/sdk/cmake")
include(CZApp)

cz_add_lvgl_app(hello_cz SOURCES src/hello_cz.c)
