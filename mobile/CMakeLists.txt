# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(MOBILE)

FILE(GLOB app_sources src/*.c)

FILE(GLOB advertise ../mylib/advertise.c)

FILE(GLOB humid ../mylib/sensors/humid.c)
FILE(GLOB press ../mylib/sensors/press.c)
FILE(GLOB temp ../mylib/sensors/temp.c)
FILE(GLOB accel ../mylib/sensors/accel.c)
FILE(GLOB light ../mylib/sensors/light.c)
FILE(GLOB tvoc ../mylib/sensors/tvoc.c)
FILE(GLOB rtc ../mylib/sensors/rtc.c)

target_sources(app PRIVATE ${app_sources} ${advertise} ${humid} ${press} ${temp} ${accel} ${light} ${tvoc} ${rtc})

target_include_directories(app PRIVATE ../mylib)

target_compile_definitions(app PRIVATE
  UUID0=${UUID0}
  UUID1=${UUID1}
  UUID2=${UUID2}
  UUID3=${UUID3}
)