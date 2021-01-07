
option(BUILD_TEST_OSPRAY "Test renderer ehich uses OSPRay" OFF)

INCLUDE_DIRECTORIES(${OSPRAY_INCLUDE_DIRS})

include_directories(${CMAKE_SOURCE_DIR}/client/)
# ------------------------------------------------------------------
# simple test tool that connects to a display wall
# ------------------------------------------------------------------
add_executable(dw2_info
  printInfo.cpp
  )
target_link_libraries(dw2_info dw2_client)

add_executable(dw2_noMPITestFrameRenderer
	noMPITestFrameRenderer.cpp
  )
target_link_libraries(dw2_noMPITestFrameRenderer
  dw2_client
  TBB::tbb
  )

if (MPI_FOUND) 
# ------------------------------------------------------------------
# test frame renderer
# ------------------------------------------------------------------
	add_executable(dw2_testFrameRenderer
	  testFrameRenderer.cpp
	  )
	target_include_directories(dw2_testFrameRenderer PRIVATE
	  ${MPI_CXX_INCLUDE_PATH}
	  )
	target_compile_options(dw2_testFrameRenderer PRIVATE
	  ${MPI_CXX_COMPILE_FLAGS}
	  )
	target_link_libraries(dw2_testFrameRenderer
	  dw2_client
	  ${MPI_CXX_LIBRARIES}
	  ${MPI_CXX_LINK_FLAGS}
      TBB::tbb
	  )

	if(BUILD_TEST_OSPRAY)

	  find_package(ospray 1.8.5 REQUIRED)

	  add_executable(dw2_testOSPRay testOSPRay.cpp)

	  target_link_libraries(dw2_testOSPRay
		MPI::MPI_CXX
		dw2_client
		#ospray::ospray
		${OSPRAY_LIBRARIES}
		${OSPRAY_COMMON_LIBRARY}
		${OSPRAY_MPI_LIBRARIES}
        TBB::tbb
		)
	endif()

 add_executable(dw2_testImages testImages.cpp)
 target_link_libraries(dw2_testImages
   MPI::MPI_CXX
   dw2_client
   TBB::tbb
 )

	option(BUILD_TEST_LANDINGGEAR "Render Landing Gear Data?" OFF)

	if(BUILD_TEST_LANDINGGEAR)
	  add_subdirectory(common)
	  include_directories(common/)
	  add_executable(dw2_testOSPRay_impi testOSPRay_impi.cpp)

	  target_link_libraries(dw2_testOSPRay_impi
		MPI::MPI_CXX
		common_libs
		dw2_client
		#ospray::ospray
		${OSPRAY_LIBRARIES}
		${OSPRAY_COMMON_LIBRARY}
		${OSPRAY_MPI_LIBRARIES}
        TBB::tbb
		)
	endif()
else()
	message(WARNING "MPI not found, only building info and no MPI tests")
endif()

