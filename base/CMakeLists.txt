# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(DT3BASE)

FILE(GLOB app_sources src/*.c)

FILE(GLOB scan ../mylib/scan.c)
FILE(GLOB node_list ../mylib/node_list.c)
FILE(GLOB wifi ../mylib/wifi.c)

target_sources(app PRIVATE ${app_sources} ${scan} ${node_list} ${wifi})

target_include_directories(app PRIVATE ../mylib)
