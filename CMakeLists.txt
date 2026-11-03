cmake_minimum_required(VERSION 3.20)
project(gxt872-rgb VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

# ---------------------------------------------------------------------------
# Core library: protocol + device discovery, no UI dependencies.
# ---------------------------------------------------------------------------
add_library(gxt872 STATIC src/core/gxt872.c)
target_include_directories(gxt872 PUBLIC src/core)

# ---------------------------------------------------------------------------
# CLI frontend
# ---------------------------------------------------------------------------
add_executable(gxt872-rgb src/cli/main.c)
target_link_libraries(gxt872-rgb PRIVATE gxt872)

# ---------------------------------------------------------------------------
# GUI frontend (Qt 6) — enabled automatically when Qt is available.
# ---------------------------------------------------------------------------
option(BUILD_GUI "Build the Qt 6 GUI" ON)
if(BUILD_GUI)
  find_package(Qt6 COMPONENTS Widgets QUIET)
  if(Qt6_FOUND)
    add_executable(gxt872-rgb-gui src/gui/main.cpp src/gui/mainwindow.cpp
                   src/gui/mainwindow.h)
    target_link_libraries(gxt872-rgb-gui PRIVATE gxt872 Qt6::Widgets)
    target_include_directories(gxt872-rgb-gui PRIVATE src/gui)
    set_target_properties(gxt872-rgb-gui PROPERTIES
      AUTOMOC ON)
  else()
    message(STATUS "Qt6 not found — GUI disabled")
  endif()
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS gxt872-rgb gxt872-rgb-gui
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/gxt872-rgb.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
# The udev rule is not installed automatically: see README (it must go to
# /etc/udev/rules.d on most distributions).
