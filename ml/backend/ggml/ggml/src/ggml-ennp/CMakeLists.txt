# Copyright (c) 2023 ESWIN Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

cmake_minimum_required(VERSION 3.16.1)
set(PROJECTNAME dsp_kernel_test)
project(${PROJECTNAME})

set(CMAKE_C_FLAGS "-Werror -O2")
set(CMAKE_CXX_FLAGS "-Werror -O2")

if(NOT DEFINED SYSROOT_PATH)
    message(FATAL_ERROR "Use -DSYSROOT_PATH=/path/to/sysroot")
endif()

set(CMAKE_SYSROOT ${SYSROOT_PATH})
set(CMAKE_C_COMPILER "$ENV{CROSS_COMPILE}gcc")
set(CMAKE_CXX_COMPILER "$ENV{CROSS_COMPILE}g++")

message(STATUS "CMAKE_SYSROOT: ${CMAKE_SYSROOT}")
message(STATUS "SYSROOT_PATH: ${SYSROOT_PATH}")
message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "KERNEL_CONFIG_DIR: ${KERNEL_CONFIG_DIR}")

set(TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${TEST_SRC} ${SYSROOT_PATH}/usr/include/essdk
                    ${SYSROOT_PATH}/usr/riscv64-linux-gnu/include/
                    ${KERNEL_CONFIG_DIR})
link_directories(${SYSROOT_PATH}/usr/lib/essdk ${SYSROOT_PATH}/usr/lib/)

file(GLOB src_files ${TEST_SRC}/*.c)

add_executable(${PROJECTNAME} ${src_files})

target_link_directories(
    ${PROJECTNAME} PUBLIC ${SYSROOT_PATH}/usr/lib/essdk
    ${SYSROOT_PATH}/usr/lib/ ${SYSROOT_PATH}/lib/riscv64-linux-gnu)

target_link_libraries(${PROJECTNAME} dl es_dsp_runtime_ll es_memory atomic)
