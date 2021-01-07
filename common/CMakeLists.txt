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
# cmake_minimum_required(VERSION 3.9)

project(dw2ClientCommonLib VERSION 1.0.0)

set (CMAKE_CXX_STANDARD 11)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)

OPTION(USE_TURBO_JPEG "Use Turbo-Jpeg compression?" OFF)

# if (USE_TURBO_JPEG)
#   add_definitions(-DTURBO_JPEG=1)
#   find_package(TurboJPEG REQUIRED)
#   # include_directories(${TURBOJPEG_INCLUDE_DIR})
#   include_directories(${JPEGTURBO_INCLUDE_DIR})
# endif()

if (USE_TURBO_JPEG) 
  add_definitions(-DTURBO_JPEG=1)
  # find libjpeg-turbo
  find_path(TURBOJPEG_INCLUDE_DIR
    NAMES turbojpeg.h
    PATHS
    $ENV{LIBJPEG_TURBO_DIR}/include
    $ENV{TURBOJPEG_DIR}/include
    #linux
    /opt/libjpeg-turbo/include
    #mac
    /usr/local/opt/jpeg-turbo/include
    )
  
  find_library(TURBOJPEG_LIBRARY
    NAMES turbojpeg
    PATHS 
    $ENV{LIBJPEG_TURBO_DIR}/lib
    $ENV{TURBOJPEG_DIR}/lib
    #linux
    /opt/libjpeg-turbo/lib
    #mac
    /usr/local/opt/jpeg-turbo/lib
    )
  include_directories(${TURBOJPEG_INCLUDE_DIR})
else()
  set(TURBOJPEG_LIBRARY "")
endif()

set(DW2_COMMON_SRC 
  Mailbox.cpp
  CompressedTile.cpp
  ServiceInfo.cpp
  Socket.cpp
  SocketGroup.cpp)

if (MPI_FOUND)
	list(APPEND DW2_COMMON_SRC mpi_util.cpp)
endif()

add_library(dw2_common STATIC ${DW2_COMMON_SRC})

if (MPI_FOUND)
	target_include_directories(dw2_common PUBLIC
	  ${MPI_CXX_INCLUDE_PATH}
	  )

  target_compile_options(dw2_common PUBLIC
	  ${MPI_CXX_COMPILE_FLAGS}
	  )

  target_link_libraries(dw2_common PUBLIC
	  ${MPI_CXX_LIBRARIES}
	  ${MPI_CXX_LINK_FLAGS})
endif()

if (WIN32)
	target_link_libraries(dw2_common PUBLIC
		Ws2_32
	)
endif()

target_include_directories(dw2_common PUBLIC
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}>
)

target_include_directories(dw2_common PUBLIC
	$<BUILD_INTERFACE:${GLFW_INCLUDE_DIR}>
)

target_compile_options(dw2_common PRIVATE
  -fPIC
  )

target_link_libraries(dw2_common PUBLIC
  TBB::tbb
  Threads::Threads
  )

if (USE_TURBO_JPEG)
target_link_libraries(dw2_common PUBLIC
  ${TURBOJPEG_LIBRARY}
  # ${JPEGTURBO_LIBRARY}
  )
endif()

include(GNUInstallDirs)

set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/dw2_common)

install(TARGETS dw2_common
    EXPORT dw2_commonTargets 
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

set_target_properties(dw2_common PROPERTIES EXPORT_NAME dw2_common)

install(EXPORT dw2_commonTargets
  FILE
    dw2_commonTargets.cmake
  NAMESPACE
    dw2_common::
  DESTINATION
    ${INSTALL_CONFIGDIR}
)

install(
  FILES
    common.h
    CompressedTile.h
    config_template.h
    Mailbox.h
    ServiceInfo.h
    Socket.h
    SocketGroup.h
    vec.h
    # cmake_generated_config_file.h
  DESTINATION
    ${CMAKE_INSTALL_INCLUDEDIR}
)

if (MPI_FOUND)
	install(
		FILES
		mpi_util.h
		DESTINATION
		${CMAKE_INSTALL_INCLUDEDIR}
	)
endif()

#Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/dw2_commonConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/dw2_commonConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/dw2_commonConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

#Install the config, configversion and custom find modules
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dw2_commonConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/dw2_commonConfigVersion.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)

##############################################
## Exporting from the build tree

export(EXPORT dw2_commonTargets FILE ${CMAKE_CURRENT_BINARY_DIR}/dw2_commonTargets.cmake NAMESPACE dw2_common::)

#Register package in user's package registry
#export(PACKAGE dw2_common)





