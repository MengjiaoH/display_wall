# ======================================================================== #
# Copyright 2019 Ingo Wald                                                 #
#                                                                          #
# Licensed under the Apache License, Version 2.0 (the "License");          #
# you may not use this file except in compliance with the License.         #
# You may obtain a copy of the License at                                  #
#                                                                          #
#     http://www.apache.org/licenses/LICENSE-2.0                           #
#                                                                          #
# Unless required by applicable law or agreed to in writing, software      #
# distributed under the License is distributed on an "AS IS" BASIS,        #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. #
# See the License for the specific language governing permissions and      #
# limitations under the License.                                           #
# ======================================================================== #

# aka "displaywald 2"
project(DW2)

cmake_minimum_required(VERSION 3.9)

set(DW2_VERSION_MAJOR 0)
set(DW2_VERSION_MINOR 1)
set(DW2_VERSION_PATCH 0)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)

  if(${CMAKE_MAJOR_VERSION} EQUAL 3)
    cmake_policy(SET CMP0042 NEW)
  endif(${CMAKE_MAJOR_VERSION} EQUAL 3)
endif(COMMAND cmake_policy)

set(CMAKE_CXX_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

configure_file(common/config_template.h ${CMAKE_BINARY_DIR}/cmake_generated_config_file.h)

# -------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(MPI)
find_package(TBB REQUIRED)

# -------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR})

if (MPI_FOUND)
	option(BUILD_SERVICE "build wall service option" OFF)
	add_definitions(-DMPI_FOUND=1)
else()
	message(WARNING "MPI not found, MPI is required for the display wall service")
endif()
option(BUILD_CLIENT "build wall service option" OFF)
option(BUILD_TESTS "build wall service option" OFF)

add_subdirectory(common)

if(BUILD_SERVICE)
  add_subdirectory(service)
endif()

if(BUILD_CLIENT)
  add_subdirectory(client)
endif()

if(BUILD_TESTS)
  add_subdirectory(tests)
endif()
