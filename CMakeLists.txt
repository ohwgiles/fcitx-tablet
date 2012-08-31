cmake_minimum_required (VERSION 2.6)

project(fcitx-tablet)

FIND_PACKAGE(Fcitx REQUIRED)
include_directories (${FCITX4_FCITX_INCLUDE_DIRS})
include_directories (${FCITX4_FCITX_CONFIG_INCLUDE_DIRS})
include_directories (${FCITX4_FCITX_UTILS_INCLUDE_DIRS})

add_definitions( -DLOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/share/locale\" -std=gnu99)

include_directories(src)
fcitx_add_addon(fcitx-tablet 
	src/pen.c
	src/ime.c
src/line.c
	src/config.c
	src/version.c
	src/drivers/lxbi.c
	src/zinnia.c
	src/fork.c)

target_link_libraries(fcitx-tablet X11 zinnia)

set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -fvisibility=hidden ${CMAKE_CXX_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_MODULE_LINKER_FLAGS}")

if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()

set(libdir ${LIB_INSTALL_DIR})

add_subdirectory(rules)
add_subdirectory(data)
