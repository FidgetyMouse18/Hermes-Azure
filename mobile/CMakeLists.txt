# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(DT3MOBILE)

FILE(GLOB app_sources src/*.c)

FILE(GLOB retransmit ../mylib/retransmit.c)

target_sources(app PRIVATE ${app_sources} ${retransmit})

target_include_directories(app PRIVATE ../mylib)
