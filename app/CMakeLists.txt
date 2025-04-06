#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

zephyr_include_directories(../include)
zephyr_include_directories(include)

add_subdirectory(drivers)
#include(drivers/CMakeLists.txt)

project(app LANGUAGES C)
set_property(TARGET app PROPERTY C_STANDARD 99)

target_link_libraries(app PRIVATE display__lcd5110)

target_sources(app PRIVATE src/main.c
                   PRIVATE src/ccs811_sensor.c
                   PRIVATE src/bme280_sensor.c
                   PRIVATE src/lcd5110_display.c)
                   #PRIVATE src/st7789v_display.c)
