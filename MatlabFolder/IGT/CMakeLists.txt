#=========================================================
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.8 FATAL_ERROR)

# Setting default INSTALL directory
# it _must_ go before project(IGT) in order to work
if(WIN32)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/INSTALL" CACHE PATH "Installation Directory")
else()
  set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation Directory")
endif()

#=========================================================
#IGT = Image Guidance Toolkit
PROJECT(IGT)
#=========================================================

#=========================================================

#CMAKE_POLICY (SET CMP0003 NEW)
#if(POLICY CMP0054)
#  CMAKE_POLICY (SET CMP0054 NEW)
#endif()
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()
if(POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW) # CMake 3.0
endif()
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW) # CMake 3.1
endif()

# Tell CMAKE to be quiet
CMAKE_POLICY (SET CMP0003 NEW)
if(POLICY CMP0054)
  CMAKE_POLICY (SET CMP0054 NEW)
endif()

## Default to release
IF (NOT CMAKE_BUILD_TYPE)
  SET (CMAKE_BUILD_TYPE Release)
ENDIF (NOT CMAKE_BUILD_TYPE)

#=========================================================

#=========================================================
SET(IGT_VERSION_MAJOR "1")
SET(IGT_VERSION_MINOR "0")
SET(IGT_VERSION_PATCH "0")

#=========================================================

#=========================================================
# Help function to debug CMake
MACRO (DD in)
  MESSAGE(${in}=[${${in}}])
ENDMACRO(DD)
#=========================================================

#=========================================================
# Output directories.
IF(NOT LIBRARY_OUTPUT_PATH)
  SET (LIBRARY_OUTPUT_PATH ${IGT_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all libraries.")
ENDIF(NOT LIBRARY_OUTPUT_PATH)
IF(NOT EXECUTABLE_OUTPUT_PATH)
  SET (EXECUTABLE_OUTPUT_PATH ${IGT_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all executables.")
ENDIF(NOT EXECUTABLE_OUTPUT_PATH)
MARK_AS_ADVANCED(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)

SET(IGT_LIBRARY_PATH "${LIBRARY_OUTPUT_PATH}")
SET(IGT_EXECUTABLE_PATH "${EXECUTABLE_OUTPUT_PATH}")
SET(CXX_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})

#=========================================================
# Remove some MS Visual c++ flags
IF(MSVC)
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
ENDIF(MSVC)

# Flag required for shared libraries
IF ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
  SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
ENDIF ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
#=========================================================

#=========================================================

# Find Qt (optional)
find_package(Qt5 COMPONENTS Widgets QUIET)
IF(Qt5_FOUND)
	SET(IGT_USE_Qt_DEFAULT ON)
ELSE()
	SET(IGT_USE_Qt_DEFAULT OFF)
ENDIF()
option(IGT_USE_Qt "Use Qt for IGT" ${IGT_USE_Qt_DEFAULT})
IF(IGT_USE_Qt)
	IF(NOT Qt_FOUND)
		find_package(Qt5 COMPONENTS Widgets PrintSupport REQUIRED)
	ENDIF()
    # Instruct CMake to run moc automatically when needed.
    set(CMAKE_AUTOMOC ON)
	
	# QCustomPlot
	include_directories(Utilities/QCustomPlot)
	LIST(APPEND IGT_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/Utilities/QCustomPlot")
ENDIF()

# GPU optional dependencies
SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/Cmake" ${CMAKE_MODULE_PATH})

option(IGT_USE_OPENCL "Use OpenCL for IGT" OFF)
if(IGT_USE_OPENCL)
  FIND_PACKAGE(OpenCL REQUIRED)
  IF(OPENCL_LIBRARIES AND OPENCL_INCLUDE_DIRS)
    INCLUDE_DIRECTORIES(${OPENCL_INCLUDE_DIRS})
    SET(OPENCL_FOUND TRUE)
  ELSE(OPENCL_LIBRARIES AND OPENCL_INCLUDE_DIRS)
    SET(OPENCL_FOUND FALSE)
  ENDIF(OPENCL_LIBRARIES AND OPENCL_INCLUDE_DIRS)
endif()

FIND_PACKAGE(CUDA_wrap)
if(CUDA_FOUND)
  SET(IGT_USE_CUDA_DEFAULT ON)
else()
  SET(IGT_USE_CUDA_DEFAULT OFF)
endif()
option(IGT_USE_CUDA "Use CUDA for IGT" ${IGT_USE_CUDA_DEFAULT})

IF(IGT_USE_CUDA)
  IF(NOT CUDA_FOUND)
    FIND_PACKAGE(CUDA_wrap REQUIRED)
  ENDIF()
  INCLUDE_DIRECTORIES(${CUDA_INCLUDE_DIRS})
  set(IGT_CUDA_PROJECTIONS_SLAB_SIZE "16" CACHE STRING "Number of projections processed simultaneously in CUDA forward and back projections")
ENDIF()

# Find ITK (required)
FIND_PACKAGE(ITK REQUIRED)
IF(ITK_FOUND)
	INCLUDE(${ITK_USE_FILE})
ELSE(ITK_FOUND)
	MESSAGE(FATAL_ERROR "Cannot build without ITK. Set ITK_DIR.")
ENDIF(ITK_FOUND)

#=========================================================

#=========================================================
# Remove some Intel compiler warnings
IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  STRING(REPLACE "-Wno-unused-parameter" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  IF(WIN32)
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qwd1268")
  ELSE(WIN32)
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd1268")
  ENDIF(WIN32)
ENDIF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
#=========================================================

#=========================================================
# Shared libraries option
OPTION(BUILD_SHARED_LIBS "Build IGT with shared libraries." ${ITK_BUILD_SHARED})
SET(IGT_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
#=========================================================

#=========================================================
# IGT uses KWStyle for checking the coding style
INCLUDE(${IGT_SOURCE_DIR}/Cmake/KWStyle/KWStyle.cmake)
#=========================================================


#=========================================================
# If choose to build documentation, then search for Doxygen executables.
OPTION(BUILD_DOXYGEN "Build Doxygen Documentation" OFF)
IF(BUILD_DOXYGEN)
  ADD_SUBDIRECTORY(Documentation/Doxygen)
ENDIF(BUILD_DOXYGEN)
#=========================================================

#=========================================================
# Installation variables
if(NOT IGT_INSTALL_RUNTIME_DIR)
  set(IGT_INSTALL_RUNTIME_DIR bin)
endif()
if(NOT IGT_INSTALL_LIB_DIR)
  set(IGT_INSTALL_LIB_DIR lib)
endif()
if(NOT IGT_INSTALL_ARCHIVE_DIR)
  set(IGT_INSTALL_ARCHIVE_DIR lib)
endif()
if(NOT IGT_INSTALL_INCLUDE_DIR)
  set(IGT_INSTALL_INCLUDE_DIR include/IGT-${IGT_VERSION_MAJOR}.${IGT_VERSION_MINOR})
endif()
if(NOT IGT_INSTALL_PACKAGE_DIR)
  set(IGT_INSTALL_PACKAGE_DIR "lib/cmake/IGT-${IGT_VERSION_MAJOR}.${IGT_VERSION_MINOR}")
endif()
#=========================================================
# Utilities
LIST(APPEND IGT_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/Code")
LIST(APPEND IGT_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/Code/RTKModules")
IF(IGT_USE_CUDA)
  LIST(APPEND IGT_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/Utilities/ITKCudaCommon/include/")
  LIST(APPEND IGT_INCLUDE_DIRS ${CUDA_TOOLKIT_INCLUDE})
ENDIF(IGT_USE_CUDA)
LIST(APPEND IGT_INCLUDE_DIRS "${PROJECT_BINARY_DIR}")
INCLUDE_DIRECTORIES(${IGT_INCLUDE_DIRS})

IF(ITK_VERSION_MINOR LESS "4")
  INCLUDE_DIRECTORIES(BEFORE Utilities/itkImageScanlineConstIterator)
ENDIF()
IF(ITK_VERSION_MINOR LESS "5")
  INCLUDE_DIRECTORIES(BEFORE Utilities/itkBinShrinkImageFilter)
ENDIF()

IF(IGT_USE_CUDA)
  ADD_SUBDIRECTORY(Utilities/ITKCudaCommon)
  INCLUDE_DIRECTORIES(Utilities/ITKCudaCommon/include)
ENDIF(IGT_USE_CUDA)

ADD_SUBDIRECTORY(Utilities/lp_solve)
include_directories(
        Utilities/lp_solve
        Utilities/lp_solve/shared
        Utilities/lp_solve/bfp
        Utilities/lp_solve/bfp/bfp_LUSOL
        Utilities/lp_solve/bfp/bfp_LUSOL/LUSOL
        Utilities/lp_solve/colamd)
LIST(APPEND IGT_INCLUDE_DIRS
    "${PROJECT_SOURCE_DIR}/Utilities/lp_solve"
    "${PROJECT_SOURCE_DIR}/Utilities/lp_solve/shared"
    "${PROJECT_SOURCE_DIR}/Utilities/lp_solve/bfp"
    "${PROJECT_SOURCE_DIR}/Utilities/lp_solve/bfp/bfp_LUSOL"
    "${PROJECT_SOURCE_DIR}/Utilities/lp_solve/bfp/bfp_LUSOL/LUSOL"
    "${PROJECT_SOURCE_DIR}/Utilities/lp_solve/colamdcode")

ADD_SUBDIRECTORY(Code)

#=========================================================
# Generate IGTConfig.cmake for the build tree.
SET (IGT_USE_FILE "${IGT_SOURCE_DIR}/Cmake/UseIGT.cmake")
SET (IGT_LIBRARY_DIRS ${IGT_LIBRARY_PATH})
CONFIGURE_FILE (Cmake/IGTConfig.cmake.in IGTConfig.cmake @ONLY)
#=========================================================

option(BUILD_APPLICATIONS "Build IGT applications" ON)
IF(BUILD_APPLICATIONS)
  ADD_SUBDIRECTORY(Applications)
ENDIF(BUILD_APPLICATIONS)

#=========================================================
# C++ Multi-thread support
option(CMAKE_USE_WIN32_THREADS_INIT "using WIN32 threads" ON)