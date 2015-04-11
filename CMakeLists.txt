cmake_minimum_required (VERSION 2.6)

project(fcitx-tablet)

FIND_PACKAGE(Fcitx REQUIRED)
include_directories (${FCITX4_FCITX_INCLUDE_DIRS})
include_directories (${FCITX4_FCITX_CONFIG_INCLUDE_DIRS})
include_directories (${FCITX4_FCITX_UTILS_INCLUDE_DIRS})

add_definitions( -DLOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/share/locale\" -std=gnu99)

set(WITH_MOUSE_WARPING FALSE CACHE BOOL "Tablet may act as mouse via XWarpPointer")
set(WITH_ZINNIA TRUE CACHE BOOL "Compile against Zinnia engine")

include_directories(src)
set(SOURCES
	src/config.c
	src/line.c
	src/tablet.c
	src/version.c
	src/drivers/gotop.c
	src/engines/fork.c)

if(WITH_ZINNIA)
	list(APPEND SOURCES src/engines/zinnia.c)
endif(WITH_ZINNIA)

fcitx_add_addon(fcitx-tablet ${SOURCES})

target_link_libraries(fcitx-tablet X11 Xfixes m)

if(WITH_ZINNIA)
	target_link_libraries(fcitx-tablet zinnia)
	set(ZINNIA_CFLAGS "-DWITH_ZINNIA")
endif(WITH_ZINNIA)

if(WITH_MOUSE_WARPING)
	set(MOUSE_WARP_CFLAGS "-DWITH_MOUSE_WARPING")
endif(WITH_MOUSE_WARPING)

set(CMAKE_C_FLAGS "-Wall -Wextra -Werror -Wno-unused-parameter -fvisibility=hidden ${MOUSE_WARP_CFLAGS} ${ZINNIA_CFLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_MODULE_LINKER_FLAGS}")

if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()

set(libdir ${LIB_INSTALL_DIR})

add_subdirectory(rules)
add_subdirectory(data)
add_subdirectory(utils)
